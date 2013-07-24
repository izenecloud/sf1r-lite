#include "IndexTaskService.h"

#include <common/JobScheduler.h>
#include <aggregator-manager/IndexWorker.h>
#include <node-manager/NodeManagerBase.h>
#include <node-manager/MasterManagerBase.h>
#include <node-manager/sharding/ScdSharder.h>
#include <node-manager/sharding/ScdDispatcher.h>
#include <node-manager/DistributeRequestHooker.h>
#include <node-manager/DistributeFileSyncMgr.h>
#include <node-manager/DistributeFileSys.h>
#include <util/driver/Request.h>

#include <glog/logging.h>
#include <boost/filesystem.hpp>

namespace bfs = boost::filesystem;
using namespace izenelib::driver;

namespace sf1r
{
static const char* SCD_BACKUP_DIR = "backup";
const static std::string DISPATCH_TEMP_DIR = "dispatch-temp-dir/";

IndexTaskService::IndexTaskService(IndexBundleConfiguration* bundleConfig)
: bundleConfig_(bundleConfig)
{
    service_ = Sf1rTopology::getServiceName(Sf1rTopology::SearchService);
}

IndexTaskService::~IndexTaskService()
{
}

bool IndexTaskService::HookDistributeRequestForIndex()
{
    Request::kCallType hooktype = (Request::kCallType)DistributeRequestHooker::get()->getHookType();
    if (hooktype == Request::FromAPI)
    {
        // from api do not need hook, just process as usually.
        return true;
    }
    const std::string& reqdata = DistributeRequestHooker::get()->getAdditionData();
    bool ret = false;
    if (hooktype == Request::FromDistribute)
    {
        indexAggregator_->distributeRequestWithoutLocal(bundleConfig_->collectionName_, "HookDistributeRequestForIndex", (int)hooktype, reqdata, ret);
    }
    else
    {
        // local hook has been moved to the request controller.
        ret = true;
    }
    if (!ret)
    {
        LOG(WARNING) << "Request failed, HookDistributeRequestForIndex failed.";
    }
    return ret;
}

bool IndexTaskService::index(unsigned int numdoc, std::string scd_path)
{
    bool result = true;

    if (DistributeFileSys::get()->isEnabled())
    {
        if (scd_path.empty())
        {
            LOG(ERROR) << "scd path should be specified while dfs is enabled.";
            return false;
        }
        scd_path = DistributeFileSys::get()->getDFSPathForLocal(scd_path);
    }
    else
    {
        scd_path = bundleConfig_->indexSCDPath();
    }

    if (bundleConfig_->isMasterAggregator() && indexAggregator_->isNeedDistribute() &&
        DistributeRequestHooker::get()->isRunningPrimary())
    {
        if (DistributeRequestHooker::get()->isHooked())
        {
            if (DistributeRequestHooker::get()->getHookType() == Request::FromDistribute)
            {
                result = distributedIndex_(numdoc, scd_path);
            }
            else
            {
                //if (DistributeFileSys::get()->isEnabled())
                //{
                //    // while dfs enabled, the master will shard the scd file under the main scd_path
                //    //  to sub-directory directly on dfs.
                //    // and the shard worker only index the sub-directory belong to it.
                //    //
                //    std::string myshard;
                //    myshard = boost::lexical_cast<std::string>(MasterManagerBase::get()->getMyShardId());
                //    scd_path = (bfs::path(scd_path)/bfs::path(DISPATCH_TEMP_DIR + myshard)).string();
                //}
                indexWorker_->index(scd_path, numdoc, result);
            }
        }
        else
        {
            task_type task = boost::bind(&IndexTaskService::distributedIndex_, this, numdoc, scd_path);
            JobScheduler::get()->addTask(task, bundleConfig_->collectionName_);
        }
    }
    else
    {
        if (bundleConfig_->isMasterAggregator() &&
            DistributeRequestHooker::get()->isRunningPrimary() && !DistributeFileSys::get()->isEnabled())
        {
            LOG(INFO) << "only local worker available, copy master scd files and indexing local.";
            // search the directory for files
            static const bfs::directory_iterator kItrEnd;
            std::string masterScdPath = bundleConfig_->masterIndexSCDPath();
            ScdParser parser(bundleConfig_->encoding_);
            bfs::path bkDir = bfs::path(masterScdPath) / SCD_BACKUP_DIR;
            LOG(INFO) << "creating directory : " << bkDir;
            bfs::create_directories(bkDir);
            for (bfs::directory_iterator itr(masterScdPath); itr != kItrEnd; ++itr)
            {
                if (bfs::is_regular_file(itr->status()))
                {
                    std::string fileName = itr->path().filename().string();
                    if (parser.checkSCDFormat(fileName))
                    {
                        try {
                        bfs::copy_file(itr->path().string(), bundleConfig_->indexSCDPath() + "/" + fileName);
                        LOG(INFO) << "SCD File copy to local index path:" << fileName;
                        LOG(INFO) << "moving SCD files to directory " << bkDir;
                            bfs::rename(itr->path().string(), bkDir / itr->path().filename());
                        }
                        catch(const std::exception& e) {
                            LOG(WARNING) << "failed to move file: " << std::endl << fileName << std::endl << e.what();
                        }
                    }
                    else
                    {
                        LOG(WARNING) << "SCD File not valid " << fileName;
                    }
                }
            }
        }
        indexWorker_->index(scd_path, numdoc, result);
    }

    return result;
}

bool IndexTaskService::isNeedSharding()
{
    if (bundleConfig_->isMasterAggregator() && indexAggregator_->isNeedDistribute() &&
        DistributeRequestHooker::get()->isRunningPrimary())
    {
        return DistributeRequestHooker::get()->getHookType() == Request::FromDistribute;
    }
    return false;
}

bool IndexTaskService::reindex_from_scd(const std::vector<std::string>& scd_list, int64_t timestamp)
{
    return indexWorker_->buildCollection(0, scd_list, timestamp);
}

bool IndexTaskService::index(boost::shared_ptr<DocumentManager>& documentManager, int64_t timestamp)
{
    if (isNeedSharding())
    {
        LOG(INFO) << "distribute the index task to all shard nodes.";
        HookDistributeRequestForIndex();
    }
 
    return indexWorker_->reindex(documentManager, timestamp);
}

bool IndexTaskService::optimizeIndex()
{
    if (isNeedSharding())
    {
        LOG(INFO) << "distribute the index task to all shard nodes.";
        HookDistributeRequestForIndex();
    }

    return indexWorker_->optimizeIndex();
}

bool IndexTaskService::createDocument(const Value& documentValue)
{
    return indexWorker_->createDocument(documentValue);
}

bool IndexTaskService::updateDocument(const Value& documentValue)
{
    return indexWorker_->updateDocument(documentValue);
}
bool IndexTaskService::updateDocumentInplace(const Value& request)
{
    return indexWorker_->updateDocumentInplace(request);
}

bool IndexTaskService::destroyDocument(const Value& documentValue)
{
    return indexWorker_->destroyDocument(documentValue);
}

void IndexTaskService::flush()
{
    indexWorker_->flush(true);
}

bool IndexTaskService::getIndexStatus(Status& status)
{
    return indexWorker_->getIndexStatus(status);
}

bool IndexTaskService::isAutoRebuild()
{
    return bundleConfig_->isAutoRebuild_;
}

std::string IndexTaskService::getScdDir(bool rebuild) const
{
    if (rebuild)
        return bundleConfig_->rebuildIndexSCDPath();
    if( bundleConfig_->isMasterAggregator() )
        return bundleConfig_->masterIndexSCDPath();
    return bundleConfig_->indexSCDPath();
}

CollectionPath& IndexTaskService::getCollectionPath() const
{
    return bundleConfig_->collPath_;
}

boost::shared_ptr<DocumentManager> IndexTaskService::getDocumentManager() const
{
    return indexWorker_->getDocumentManager();
}

bool IndexTaskService::distributedIndex_(unsigned int numdoc, std::string scd_dir)
{
    // notify that current master is indexing for the specified collection,
    // we may need to check that whether other Master it's indexing this collection in some cases,
    // or it's depends on Nginx router strategy.
    MasterManagerBase::get()->registerIndexStatus(bundleConfig_->collectionName_, true);

    if (!DistributeFileSys::get()->isEnabled())
    {
        scd_dir = bundleConfig_->masterIndexSCDPath();
    }

    bool ret = distributedIndexImpl_(
                    numdoc,
                    bundleConfig_->collectionName_,
                    scd_dir);

    MasterManagerBase::get()->registerIndexStatus(bundleConfig_->collectionName_, false);

    return ret;
}

bool IndexTaskService::distributedIndexImpl_(
    unsigned int numdoc,
    const std::string& collectionName,
    const std::string& masterScdPath)
{
    if (!scdSharder_)
    {
        if (!createScdSharder(scdSharder_))
        {
            LOG(ERROR) << "create scd sharder failed.";
            return false;
        }
        if (!scdSharder_)
        {
            LOG(INFO) << "no scd sharder!";
            return false;
        }
    }

    std::string scd_dir = masterScdPath;
    std::vector<std::string> outScdFileList;
    //
    // 1. dispatching scd to multiple nodes
    if (!DistributeFileSys::get()->isEnabled())
    {
        boost::shared_ptr<ScdDispatcher> scdDispatcher(new BatchScdDispatcher(scdSharder_,
                collectionName, DistributeFileSys::get()->isEnabled()));
        if(!scdDispatcher->dispatch(outScdFileList, masterScdPath, numdoc))
            return false;
    }
    // 2. send index request to multiple nodes
    LOG(INFO) << "start distributed indexing";
    HookDistributeRequestForIndex();

    if (!DistributeFileSys::get()->isEnabled())
        scd_dir = bundleConfig_->indexSCDPath();
    //else
    //{
    //    std::string myshard;
    //    myshard = boost::lexical_cast<std::string>(MasterManagerBase::get()->getMyShardId());
    //    scd_dir = (bfs::path(scd_dir)/bfs::path(DISPATCH_TEMP_DIR + myshard)).string();
    //}
    bool ret = true;
    
    // starting local index.
    indexWorker_->index(scd_dir, numdoc, ret);

    if (ret && !DistributeFileSys::get()->isEnabled())
    {
        bfs::path bkDir = bfs::path(masterScdPath) / SCD_BACKUP_DIR;
        bfs::create_directories(bkDir);
        LOG(INFO) << "moving " << outScdFileList.size() << " SCD files to directory " << bkDir;
        for (size_t i = 0; i < outScdFileList.size(); i++)
        {
            try {
                bfs::rename(outScdFileList[i], bkDir / bfs::path(outScdFileList[i]).filename());
            }
            catch(const std::exception& e) {
                LOG(WARNING) << "failed to move file: " << std::endl << outScdFileList[i] << std::endl << e.what();
            }
        }
    }

    return ret;
}

bool IndexTaskService::createScdSharder(
    boost::shared_ptr<ScdSharder>& scdSharder)
{
    if (bundleConfig_->indexShardKeys_.empty())
    {
        LOG(ERROR) << "No sharding key!";
        return false;
    }

    // sharding configuration
    if (MasterManagerBase::get()->getCollectionShardids(service_,
            bundleConfig_->collectionName_, shard_cfg_.shardidList_))
    {
        //cfg.shardNum_ = cfg.shardidList_.size();
        //cfg.totalShardNum_ = NodeManagerBase::get()->getTotalShardNum();
    }
    else
    {
        LOG(ERROR) << "No shardid configured for " << bundleConfig_->collectionName_;
        return false;
    }

    ShardingConfig::RangeListT ranges;
    ranges.push_back(100);
    ranges.push_back(1000);
    shard_cfg_.addRangeShardKey("UnchangableRangeProperty", ranges);
    ShardingConfig::AttrListT strranges;
    strranges.push_back("abc");
    shard_cfg_.addAttributeShardKey("UnchangableAttributeProperty", strranges);
    shard_cfg_.setUniqueShardKey(bundleConfig_->indexShardKeys_[0]);

    scdSharder.reset(new ScdSharder);

    if (scdSharder && scdSharder->init(shard_cfg_))
        return true;
    else
        return false;
}

izenelib::util::UString::EncodingType IndexTaskService::getEncode() const
{
    return bundleConfig_->encoding_;
}

}

#include "IndexTaskService.h"

#include <common/JobScheduler.h>
#include <aggregator-manager/IndexWorker.h>
#include <node-manager/NodeManagerBase.h>
#include <node-manager/MasterManagerBase.h>
#include <node-manager/sharding/ScdSharder.h>
#include <node-manager/sharding/ShardingStrategy.h>
#include <node-manager/sharding/ScdDispatcher.h>
#include <node-manager/DistributeRequestHooker.h>
#include <node-manager/DistributeFileSyncMgr.h>
#include <util/driver/Request.h>

#include <glog/logging.h>
#include <boost/filesystem.hpp>

namespace bfs = boost::filesystem;
using namespace izenelib::driver;

namespace sf1r
{
static const char* SCD_BACKUP_DIR = "backup";

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

bool IndexTaskService::index(unsigned int numdoc)
{
    bool result = true;

    if (bundleConfig_->isMasterAggregator() && indexAggregator_->isNeedDistribute() &&
        DistributeRequestHooker::get()->isRunningPrimary())
    {
        if (DistributeRequestHooker::get()->isHooked())
        {
            if (DistributeRequestHooker::get()->getHookType() == Request::FromDistribute)
            {
                result = distributedIndex_(numdoc);
            }
            else
            {
                indexWorker_->index(numdoc, result);
            }
        }
        else
        {
            task_type task = boost::bind(&IndexTaskService::distributedIndex_, this, numdoc);
            JobScheduler::get()->addTask(task, bundleConfig_->collectionName_);
        }
    }
    else
    {
        if (bundleConfig_->isMasterAggregator() &&
            DistributeRequestHooker::get()->isRunningPrimary())
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
        indexWorker_->index(numdoc, result);
    }

    return result;
}

bool IndexTaskService::reindex_from_scd(const std::vector<std::string>& scd_list, int64_t timestamp)
{
    return indexWorker_->buildCollection(0, scd_list, timestamp);
}

bool IndexTaskService::index(boost::shared_ptr<DocumentManager>& documentManager, int64_t timestamp)
{
    return indexWorker_->reindex(documentManager, timestamp);
}

bool IndexTaskService::optimizeIndex()
{
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

bool IndexTaskService::reload()
{
    return indexWorker_->reload();
}

bool IndexTaskService::getIndexStatus(Status& status)
{
    return indexWorker_->getIndexStatus(status);
}

bool IndexTaskService::isAutoRebuild()
{
    return bundleConfig_->isAutoRebuild_;
}

uint32_t IndexTaskService::getDocNum()
{
    return indexWorker_->getDocNum();
}

uint32_t IndexTaskService::getKeyCount(const std::string& property_name)
{
    return indexWorker_->getKeyCount(property_name);
}

std::string IndexTaskService::getScdDir(bool rebuild) const
{
    if (rebuild)
        return bundleConfig_->rebuildIndexSCDPath();
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

bool IndexTaskService::distributedIndex_(unsigned int numdoc)
{
    // notify that current master is indexing for the specified collection,
    // we may need to check that whether other Master it's indexing this collection in some cases,
    // or it's depends on Nginx router strategy.
    MasterManagerBase::get()->registerIndexStatus(bundleConfig_->collectionName_, true);

    bool ret = distributedIndexImpl_(
                    numdoc,
                    bundleConfig_->collectionName_,
                    bundleConfig_->masterIndexSCDPath(),
                    bundleConfig_->indexShardKeys_);

    MasterManagerBase::get()->registerIndexStatus(bundleConfig_->collectionName_, false);

    return ret;
}

bool IndexTaskService::distributedIndexImpl_(
    unsigned int numdoc,
    const std::string& collectionName,
    const std::string& masterScdPath,
    const std::vector<std::string>& shardKeyList)
{
    // 1. dispatching scd to multiple nodes
    boost::shared_ptr<ScdSharder> scdSharder;
    if (!createScdSharder(scdSharder, shardKeyList))
        return false;

    boost::shared_ptr<ScdDispatcher> scdDispatcher(new BatchScdDispatcher(scdSharder, collectionName));
    std::vector<std::string> outScdFileList;
    if(!scdDispatcher->dispatch(outScdFileList, masterScdPath, numdoc))
        return false;

    // 2. send index request to multiple nodes
    LOG(INFO) << "start distributed indexing";
    HookDistributeRequestForIndex();
    bool ret = true;
    
    //indexAggregator_->distributeRequest(collectionName, "index", numdoc, ret);
    indexWorker_->index(numdoc, ret);

    if (ret)
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
    boost::shared_ptr<ScdSharder>& scdSharder,
    const std::vector<std::string>& shardKeyList)
{
    if (shardKeyList.empty())
    {
        LOG(ERROR) << "No sharding key!";
        return false;
    }

    // sharding configuration
    ShardingConfig cfg;
    if (MasterManagerBase::get()->getCollectionShardids(service_, bundleConfig_->collectionName_, cfg.shardidList_))
    {
        cfg.shardNum_ = cfg.shardidList_.size();
        cfg.totalShardNum_ = NodeManagerBase::get()->getTotalShardNum();
    }
    else
    {
        LOG(ERROR) << "No shardid configured for " << bundleConfig_->collectionName_;
        return false;
    }
    for (size_t i = 0; i < shardKeyList.size(); i++)
    {
        cfg.addShardKey(shardKeyList[i]);
        LOG(INFO) << "Shard Key: " << shardKeyList[i];
    }
    cfg.setShardStrategy(ShardingConfig::SHARDING_HASH); // use proper strategy

    scdSharder.reset(new ScdSharder);

    if (scdSharder && scdSharder->init(cfg))
        return true;
    else
        return false;
}

izenelib::util::UString::EncodingType IndexTaskService::getEncode() const
{
    return bundleConfig_->encoding_;
}

}

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
#include <common/Utilities.h>

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

    //ShardingConfig::RangeListT ranges;
    //ranges.push_back(100);
    //ranges.push_back(1000);
    //shard_cfg_.addRangeShardKey("UnchangableRangeProperty", ranges);
    //ShardingConfig::AttrListT strranges;
    //strranges.push_back("abc");
    //shard_cfg_.addAttributeShardKey("UnchangableAttributeProperty", strranges);

    if (DistributeFileSys::get()->isEnabled() && !bundleConfig_->indexShardKeys_.empty())
    {
        const std::string& coll = bundleConfig_->collectionName_;
        sharding_map_dir_ = DistributeFileSys::get()->getFixedCopyPath("/sharding_map/");
        sharding_map_dir_ = DistributeFileSys::get()->getDFSPathForLocal(sharding_map_dir_);
        if (!bfs::exists(sharding_map_dir_))
        {
            bfs::create_directories(sharding_map_dir_);
        }
        sharding_strategy_.reset(new MapShardingStrategy(sharding_map_dir_ + coll));
        sharding_strategy_->shard_cfg_.shardidList_ = bundleConfig_->col_shard_info_.shardList_;
        if (sharding_strategy_->shard_cfg_.shardidList_.empty())
            throw "sharding config is error!";
        sharding_strategy_->shard_cfg_.setUniqueShardKey(bundleConfig_->indexShardKeys_[0]);
        sharding_strategy_->init();
    }
}

IndexTaskService::~IndexTaskService()
{
}

std::string IndexTaskService::getShardingMapDir()
{
    return sharding_map_dir_;
}

bool IndexTaskService::SendRequestToSharding(uint32_t shardid)
{
    Request::kCallType hooktype = (Request::kCallType)DistributeRequestHooker::get()->getHookType();
    if (hooktype == Request::FromAPI)
    {
        return true;
    }
    const std::string& reqdata = DistributeRequestHooker::get()->getAdditionData();
    bool ret = false;
    if (hooktype == Request::FromDistribute)
    {
        indexAggregator_->singleRequest(bundleConfig_->collectionName_, 0,
            "HookDistributeRequestForIndex", (int)hooktype, reqdata, ret, shardid);
    }
    else
    {
        ret = true;
    }
    if (!ret)
    {
        LOG(WARNING) << "Send Request to shard node failed.";
    }
    return ret;
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
        indexAggregator_->distributeRequestWithoutLocal(bundleConfig_->collectionName_, 0, "HookDistributeRequestForIndex", (int)hooktype, reqdata, ret);
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

bool IndexTaskService::index(unsigned int numdoc, std::string scd_path, int disable_sharding_type)
{
    bool result = true;

    // disable_sharding_type , 0 no disable, 1 disable sending request to sharding node,
    // 2 disable sending to sharding node and disable sharding SCD on local. (This means
    //  local node will index all of the docs in the given SCD files.)
    bool disable_sharding = (disable_sharding_type != (int)DefaultShard);

    indexWorker_->disableSharding(disable_sharding_type == (int)LocalOnly);

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
            if (DistributeRequestHooker::get()->getHookType() == Request::FromDistribute &&
                !disable_sharding)
            {
                result = distributedIndex_(numdoc, scd_path);
            }
            else
            {
                if (disable_sharding)
                    LOG(INFO) << "==== The sharding is disabled! =====";
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
                
                if (!isNeedDoLocal())
                    return false;
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

        if (!isNeedDoLocal())
            return false;
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

bool IndexTaskService::isNeedDoLocal()
{
    if (NodeManagerBase::get()->isDistributed() && !bundleConfig_->isWorkerNode())
    {
        LOG(INFO) << "local worker is disabled, no need do local work.";
        return false;
    }
    return true;
}

bool IndexTaskService::reindex_from_scd(const std::vector<std::string>& scd_list, int64_t timestamp)
{
    if (!isNeedDoLocal())
        return false;
    indexWorker_->disableSharding(false);
    return indexWorker_->buildCollection(0, scd_list, timestamp);
}

bool IndexTaskService::index(boost::shared_ptr<DocumentManager>& documentManager, int64_t timestamp)
{
    if (!isNeedDoLocal())
        return false;
    return indexWorker_->reindex(documentManager, timestamp);
}

bool IndexTaskService::optimizeIndex()
{
    if (!isNeedDoLocal())
        return false;
    return indexWorker_->optimizeIndex();
}

bool IndexTaskService::createDocument(const Value& documentValue)
{
    if (isNeedSharding())
    {
        if (!scdSharder_)
            createScdSharder(scdSharder_);
        SCDDoc scddoc;
        IndexWorker::value2SCDDoc(documentValue, scddoc);
        shardid_t shardid = scdSharder_->sharding(scddoc);
        if (shardid != MasterManagerBase::get()->getMyShardId())
        {
            // need send to the shard.
            return SendRequestToSharding(shardid);
        }
    }
    return indexWorker_->createDocument(documentValue);
}

bool IndexTaskService::updateDocument(const Value& documentValue)
{
    if (isNeedSharding())
    {
        if (!scdSharder_)
            createScdSharder(scdSharder_);
        SCDDoc scddoc;
        IndexWorker::value2SCDDoc(documentValue, scddoc);
        shardid_t shardid = scdSharder_->sharding(scddoc);
        if (shardid != MasterManagerBase::get()->getMyShardId())
        {
            // need send to the shard.
            return SendRequestToSharding(shardid);
        }
    }

    return indexWorker_->updateDocument(documentValue);
}
bool IndexTaskService::updateDocumentInplace(const Value& request)
{
    if (isNeedSharding())
    {
        if (!scdSharder_)
            createScdSharder(scdSharder_);
        SCDDoc scddoc;
        IndexWorker::value2SCDDoc(request, scddoc);
        shardid_t shardid = scdSharder_->sharding(scddoc);
        if (shardid != MasterManagerBase::get()->getMyShardId())
        {
            // need send to the shard.
            return SendRequestToSharding(shardid);
        }
    }

    return indexWorker_->updateDocumentInplace(request);
}

bool IndexTaskService::destroyDocument(const Value& documentValue)
{
    if (isNeedSharding())
    {
        if (!scdSharder_)
            createScdSharder(scdSharder_);
        SCDDoc scddoc;
        IndexWorker::value2SCDDoc(documentValue, scddoc);
        shardid_t shardid = scdSharder_->sharding(scddoc);
        if (shardid != MasterManagerBase::get()->getMyShardId())
        {
            // need send to the shard.
            return SendRequestToSharding(shardid);
        }
    }

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

    if (!MasterManagerBase::get()->isAllShardNodeOK(bundleConfig_->col_shard_info_.shardList_))
    {
        LOG(ERROR) << "some of sharding node is not ready for index.";
        return false;
    }

    std::string scd_dir = masterScdPath;
    std::vector<std::string> outScdFileList;
    //
    // 1. dispatching scd to multiple nodes
    if (!DistributeFileSys::get()->isEnabled())
    {
        boost::shared_ptr<ScdDispatcher> scdDispatcher(new BatchScdDispatcher(scdSharder_,
                collectionName, DistributeFileSys::get()->isEnabled()));
        if(!scdDispatcher->dispatch(outScdFileList, masterScdPath, bundleConfig_->indexSCDPath(), numdoc))
            return false;
    }
    // 2. send index request to multiple nodes
    LOG(INFO) << "start distributed indexing";
    HookDistributeRequestForIndex();

    if (!DistributeFileSys::get()->isEnabled())
        scd_dir = bundleConfig_->indexSCDPath();

    bool ret = true;
    if (isNeedDoLocal())
    {
        // starting local index.
        indexWorker_->index(scd_dir, numdoc, ret);
    }
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

    if (!isNeedDoLocal())
        return false;
    return ret;
}

bool IndexTaskService::createScdSharder(
    boost::shared_ptr<ScdSharder>& scdSharder)
{
    return indexWorker_->createScdSharder(scdSharder);
}

izenelib::util::UString::EncodingType IndexTaskService::getEncode() const
{
    return bundleConfig_->encoding_;
}

const std::vector<shardid_t>& IndexTaskService::getShardidListForSearch()
{
    return bundleConfig_->col_shard_info_.shardList_;
}

typedef std::map<shardid_t, std::vector<vnodeid_t> > ShardingTopologyT;
static void printSharding(const ShardingTopologyT& sharding_topology)
{
    for(ShardingTopologyT::const_iterator cit = sharding_topology.begin();
        cit != sharding_topology.end(); ++cit)
    {
        std::cout << "sharding : " << (uint32_t)cit->first << " is holding : ";
        for (size_t i = 0; i < cit->second.size(); ++i)
        {
            std::cout << cit->second[i] << ", ";
        }
        std::cout << std::endl;
    }
}

static bool removeSharding(const std::vector<shardid_t>& remove_sharding_nodes,
    std::vector<shardid_t>& left_sharding_nodes,
    size_t new_vnode_for_sharding,
    ShardingTopologyT& current_sharding_topology,
    std::map<vnodeid_t, std::pair<shardid_t, shardid_t> >& migrate_data_list,
    std::vector<shardid_t>& sharding_map)
{
    ShardingTopologyT remove_sharding_topo;
    for (size_t i = 0; i < remove_sharding_nodes.size(); ++i)
    {
        remove_sharding_topo[remove_sharding_nodes[i]] = current_sharding_topology[remove_sharding_nodes[i]];
        current_sharding_topology.erase(remove_sharding_nodes[i]);
    }

    if (current_sharding_topology.empty() || remove_sharding_topo.size() != remove_sharding_nodes.size())
    {
        return false;
    }
    ShardingTopologyT::iterator migrate_to_it = current_sharding_topology.begin();
    for(ShardingTopologyT::iterator remove_it = remove_sharding_topo.begin();
        remove_it != remove_sharding_topo.end(); ++remove_it)
    {
        size_t vnode_index = 0;
        for (; vnode_index < remove_it->second.size();
            ++vnode_index)
        {
            vnodeid_t vid = remove_it->second[vnode_index];
            migrate_data_list[vid] = std::pair<shardid_t, shardid_t>(remove_it->first, migrate_to_it->first);
            LOG(INFO) << "vnode : " << vid << " will be moved from "
                << (uint32_t)remove_it->first << " to " << (uint32_t)migrate_to_it->first;
            sharding_map[vid] = migrate_to_it->first;
            migrate_to_it->second.push_back(vid);
            if (migrate_to_it->second.size() > new_vnode_for_sharding)
            {
                // the new sharding node got enough data. move to next.
                left_sharding_nodes.push_back(migrate_to_it->first);
                ++migrate_to_it;
                if (migrate_to_it == current_sharding_topology.end())
                {
                    ShardingTopologyT::iterator tmpit = remove_it;
                    if (vnode_index != (remove_it->second.size() - 1) ||
                        ++tmpit != remove_sharding_topo.end() )
                    {
                        LOG(INFO) << "the remove sharding vnode can not totally migrate to others.";
                        return false;
                    }
                    break;
                }
            }
        }
        remove_it->second.clear();
    }

    while(migrate_to_it != current_sharding_topology.end())
    {
        left_sharding_nodes.push_back(migrate_to_it->first);
        ++migrate_to_it;
    }
    return true;
}

static void migrateSharding(const std::vector<shardid_t>& new_sharding_nodes,
    size_t new_vnode_for_sharding,
    ShardingTopologyT& current_sharding_topology,
    ShardingTopologyT& new_sharding_topology,
    std::map<vnodeid_t, std::pair<shardid_t, shardid_t> >& migrate_data_list,
    std::vector<shardid_t>& sharding_map)
{
    // move the vnode from src sharding node to dest sharding node.
    size_t migrate_to = 0;

    for(ShardingTopologyT::iterator it = current_sharding_topology.begin();
        it != current_sharding_topology.end(); ++it)
    {
        size_t migrate_start = it->second.size() - 1;
        while (migrate_to < new_sharding_nodes.size())
        {
            // this old sharding has not enough data so do not migrate from this node.
            if (it->second.size() <= new_vnode_for_sharding)
                break;
            size_t old_start = migrate_start;
            for (size_t vnode_index = old_start;
                vnode_index > new_vnode_for_sharding;
                --vnode_index)
            {
                vnodeid_t vid = it->second[vnode_index];
                migrate_data_list[vid] = std::pair<shardid_t, shardid_t>(it->first, new_sharding_nodes[migrate_to]);
                LOG(INFO) << "vnode : " << vid << " will be moved from " << (uint32_t)it->first << " to " << (uint32_t)new_sharding_nodes[migrate_to];
                sharding_map[vid] = new_sharding_nodes[migrate_to];
                new_sharding_topology[new_sharding_nodes[migrate_to]].push_back(vid);
                --migrate_start;
                if (new_sharding_topology[new_sharding_nodes[migrate_to]].size() >= new_vnode_for_sharding)
                {
                    // the new sharding node got enough data. move to next.
                    ++migrate_to;
                    if (migrate_to >= new_sharding_nodes.size())
                        break;
                }
            }
            it->second.erase(it->second.begin() + migrate_start, it->second.end());
        }
        if (migrate_to >= new_sharding_nodes.size())
            break;
    }
}

bool IndexTaskService::doMigrateWork(bool removing,
    const std::map<vnodeid_t, std::pair<shardid_t, shardid_t> >& migrate_data_list,
    const std::vector<shardid_t>& migrate_nodes,
    const std::string& map_file,
    const std::vector<shardid_t>& current_sharding_map)
{
    std::map<std::string, std::map<shardid_t, std::vector<vnodeid_t> > > migrate_from_to_list;
    for(std::map<vnodeid_t, std::pair<shardid_t, shardid_t> >::const_iterator cit = migrate_data_list.begin();
        cit != migrate_data_list.end(); ++cit)
    {
        std::string shardip = MasterManagerBase::get()->getShardNodeIP(cit->second.first);
        if (shardip.empty())
        {
            LOG(ERROR) << "get source shard node ip error. " << cit->second.first;
            return false;
        }
        migrate_from_to_list[shardip][cit->second.second].push_back(cit->first);
    }

    std::map<shardid_t, std::vector<std::string> > generated_insert_scds;
    // the scds used for remove on the src node.
    std::map<shardid_t, std::vector<std::string> > generated_del_scds;
    bool ret = true;
    ret = DistributeFileSyncMgr::get()->generateMigrateScds(bundleConfig_->collectionName_,
        migrate_from_to_list,
        generated_insert_scds,
        generated_del_scds);

    if (!ret)
    {
        LOG(ERROR) << "generate the migrate SCD files failed.";
        return false;
    }

    // wait for all sharding nodes to finish their write queue.
    if(!MasterManagerBase::get()->waitForMigrateReady(getShardidListForSearch()))
    {
        LOG(INFO) << " wait for migrate get ready failed.";
        return false;
    }

    if (!removing)
    {
        // wait new sharding nodes to started.
        if (!MasterManagerBase::get()->waitForNewShardingNodes(migrate_nodes))
        {
            LOG(INFO) << "wait for new sharding nodes to startup failed.";
            return false;
        }
    }

    if (!indexShardingNodes(generated_insert_scds))
    {
        return false;
    }

    if (!removing)
    {
        // wait for the new sharding nodes to finish indexing.
        MasterManagerBase::get()->waitForMigrateIndexing(migrate_nodes);
    }

    indexShardingNodes(generated_del_scds);
    MasterManagerBase::get()->waitForMigrateIndexing(getShardidListForSearch());

    MapShardingStrategy::saveShardingMapToFile(map_file, current_sharding_map);
    // update config will cause the collection to restart, so 
    // the IndexTaskService will be destructed.
    updateShardingConfig(migrate_nodes, removing);
    return true;
}

bool IndexTaskService::addNewShardingNodes(const std::vector<shardid_t>& new_sharding_nodes)
{
    if (!bundleConfig_->isMasterAggregator())
    {
        LOG(INFO) << "change sharding node must be send to the master node.";
        return false;
    }
    if (!sharding_strategy_ || sharding_strategy_->shard_cfg_.shardidList_.size() == 0)
    {
        LOG(ERROR) << "no sharding config.";
        return false;
    }
    if (new_sharding_nodes.empty())
    {
        LOG(INFO) << "empty new sharding nodes.";
        return false;
    }
    for (size_t i = 0; i < new_sharding_nodes.size(); ++i)
    {
        if (std::find(bundleConfig_->col_shard_info_.shardList_.begin(),
                bundleConfig_->col_shard_info_.shardList_.end(),
                new_sharding_nodes[i]) != bundleConfig_->col_shard_info_.shardList_.end())
        {
            LOG(ERROR) << "new sharding nodes exists in the old sharding config.";
            return false;
        }
    }

    size_t current_sharding_num = sharding_strategy_->shard_cfg_.shardidList_.size();
    // reading current sharding config(include the load on these nodes), 
    // and determine which nodes need 
    // migrate which part of their data.
    std::vector<shardid_t> current_sharding_map;
    std::string map_file = sharding_map_dir_ + bundleConfig_->collectionName_;
    MapShardingStrategy::readShardingMapFile(map_file, current_sharding_map);

    if (bundleConfig_->col_shard_info_.shardList_.size() == 1)
    {
        if (current_sharding_map.empty())
        {
            current_sharding_map.resize(MapShardingStrategy::MAX_MAP_SIZE, bundleConfig_->col_shard_info_.shardList_[0]);
        }
    }

    if (current_sharding_map.empty())
    {
        LOG(ERROR) << "sharding map is empty!";
        return false;
    }
    if (current_sharding_map.size() < current_sharding_num + new_sharding_nodes.size())
    {
        LOG(ERROR) << "the actual sharding num is larger than virtual nodes." << current_sharding_map.size();
        return false;
    }
    size_t current_vnode_for_sharding = current_sharding_map.size()/current_sharding_num;
    size_t new_vnode_for_sharding = current_sharding_map.size()/(current_sharding_num + new_sharding_nodes.size());
    ShardingTopologyT current_sharding_topology;
    for (size_t i = 0; i < current_sharding_map.size(); ++i)
    {
        current_sharding_topology[current_sharding_map[i]].push_back(i);
    }

    LOG(INFO) << "Before migrate, the average vnodes for each sharding is: " << current_vnode_for_sharding
       << " and sharding topology is : ";
    printSharding(current_sharding_topology);

    // move the vnode from src sharding node to dest sharding node.
    std::map<vnodeid_t, std::pair<shardid_t, shardid_t> > migrate_data_list;
    ShardingTopologyT new_sharding_topology;

    migrateSharding(new_sharding_nodes, new_vnode_for_sharding,
        current_sharding_topology,
        new_sharding_topology, migrate_data_list,
        current_sharding_map);

    LOG(INFO) << "After migrate, the average vnodes for each sharding will be: " << new_vnode_for_sharding 
       << " and sharding topology will be : ";
    printSharding(current_sharding_topology);
    printSharding(new_sharding_topology);

    // this will disallow any new write. This may fail if other migrate 
    // running or not all sharding nodes alive.
    if(!MasterManagerBase::get()->notifyAllShardingBeginMigrate(getShardidListForSearch()))
    {
        return false;
    }

    bool ret = doMigrateWork(false, migrate_data_list, new_sharding_nodes,
        map_file, current_sharding_map);
    
    // allow new write running.
    MasterManagerBase::get()->notifyAllShardingEndMigrate();

    return ret;
}

bool IndexTaskService::indexShardingNodes(const std::map<shardid_t, std::vector<std::string> >& generated_migrate_scds)
{
    std::string tmp_migrate_scd_dir = DistributeFileSys::get()->getFixedCopyPath("/migrate_scds/"
        + bundleConfig_->collectionName_ + "/" + boost::lexical_cast<std::string>(Utilities::createTimeStamp()));

    bfs::create_directories(DistributeFileSys::get()->getDFSPathForLocal(tmp_migrate_scd_dir));

    std::map<shardid_t, std::vector<std::string> >::const_iterator cit = generated_migrate_scds.begin();
    for (; cit != generated_migrate_scds.end(); ++cit)
    {
        LOG(INFO) << "prepare scd files for sharding node : " << (uint32_t)cit->first;
        std::string shard_scd_dir = tmp_migrate_scd_dir + "/shard" + getShardidStr(cit->first) + "/";
        bfs::create_directories(DistributeFileSys::get()->getDFSPathForLocal(shard_scd_dir));
        for (size_t i = 0; i < cit->second.size(); ++i)
        {
            LOG(INFO) << "add migrate scd : " << cit->second[i];
            bfs::rename(DistributeFileSys::get()->getDFSPathForLocal(cit->second[i]),
                bfs::path(DistributeFileSys::get()->getDFSPathForLocal(shard_scd_dir))/(bfs::path(cit->second[i]).filename()));
        }

        // send index command.
        std::string json_req = "{\"collection\":\"" + bundleConfig_->collectionName_
            + "\",\"index_scd_path\":\"" + shard_scd_dir
            + "\",\"disable_sharding\":2"
            + ",\"header\":{\"action\":\"index\",\"controller\":\"commands\"},\"uri\":\"commands/index\"}";

        LOG(INFO) << "send request : " << json_req;
        std::vector<shardid_t> shardid;
        shardid.push_back(cit->first);
        if (!MasterManagerBase::get()->pushWriteReqToShard(json_req, shardid, true, true))
        {
            LOG(WARNING) << "push write failed.";
            return false;
        }
    }
    return true;
}

void IndexTaskService::updateShardingConfig(const std::vector<shardid_t>& new_sharding_nodes, bool removing)
{
    std::string sharding_cfg;
    std::vector<shardid_t> curr_shard_nodes = getShardidListForSearch();
    if (!removing)
    {
        for (size_t i = 0; i < curr_shard_nodes.size(); ++i)
        {
            if (sharding_cfg.empty())
                sharding_cfg = getShardidStr(curr_shard_nodes[i]);
            else
                sharding_cfg += "," + getShardidStr(curr_shard_nodes[i]);
        }
    }
    for (size_t i = 0; i < new_sharding_nodes.size(); ++i)
    {
        if (sharding_cfg.empty())
            sharding_cfg = getShardidStr(new_sharding_nodes[i]);
        else
            sharding_cfg += "," + getShardidStr(new_sharding_nodes[i]);
    }
    LOG(INFO) << "new sharding cfg is : " << sharding_cfg;
    //
    // send index command.
    std::string json_req = "{\"collection\":\"" + bundleConfig_->collectionName_
        + "\",\"new_sharding_cfg\":\"" + sharding_cfg
        + "\",\"header\":{\"action\":\"update_sharding_conf\",\"controller\":\"collection\"},\"uri\":\"collection/update_sharding_conf\"}";

    LOG(INFO) << "send request : " << json_req;
    if (!removing)
        MasterManagerBase::get()->pushWriteReqToShard(json_req, new_sharding_nodes, true, true);
    if (bundleConfig_->isMasterAggregator() && !bundleConfig_->isWorkerNode())
    {
        curr_shard_nodes.push_back(MasterManagerBase::get()->getMyShardId());
    }
    MasterManagerBase::get()->pushWriteReqToShard(json_req, curr_shard_nodes, true, true);
}

bool IndexTaskService::generateMigrateSCD(const std::map<shardid_t, std::vector<vnodeid_t> >& vnode_list,
    std::map<shardid_t, std::string>& generated_insert_scds,
    std::map<shardid_t, std::string>& generated_del_scds)
{
    return indexWorker_->generateMigrateSCD(vnode_list, generated_insert_scds, generated_del_scds);
}

bool IndexTaskService::removeShardingNodes(const std::vector<shardid_t>& remove_sharding_nodes)
{
    if (!bundleConfig_->isMasterAggregator())
    {
        LOG(INFO) << "change sharding node must be send to the master node.";
        return false;
    }

    if (!sharding_strategy_ || sharding_strategy_->shard_cfg_.shardidList_.size() == 0)
    {
        LOG(ERROR) << "no sharding config.";
        return false;
    }
    if (remove_sharding_nodes.empty())
    {
        LOG(INFO) << "empty sharding nodes.";
        return false;
    }

    size_t current_sharding_num = sharding_strategy_->shard_cfg_.shardidList_.size();
    if (current_sharding_num == 1)
    {
        LOG(INFO) << "The last one sharding node can not be removed.";
        return false;
    }

    if (remove_sharding_nodes.size() >= current_sharding_num)
    {
        LOG(INFO) << "You can not remove all sharding nodes.";
        return false;
    }

    for (size_t i = 0; i < remove_sharding_nodes.size(); ++i)
    {
        if (std::find(bundleConfig_->col_shard_info_.shardList_.begin(),
                bundleConfig_->col_shard_info_.shardList_.end(),
                remove_sharding_nodes[i]) == bundleConfig_->col_shard_info_.shardList_.end())
        {
            LOG(ERROR) << "removing sharding node does not exist in the old sharding config.";
            return false;
        }
    }

    std::vector<shardid_t> current_sharding_map;
    std::string map_file = sharding_map_dir_ + bundleConfig_->collectionName_;
    MapShardingStrategy::readShardingMapFile(map_file, current_sharding_map);

    if (current_sharding_map.empty())
    {
        LOG(ERROR) << "sharding map is empty!";
        return false;
    }

    size_t current_vnode_for_sharding = current_sharding_map.size()/current_sharding_num;
    size_t new_vnode_for_sharding = current_sharding_map.size()/(current_sharding_num - remove_sharding_nodes.size());

    ShardingTopologyT current_sharding_topology;
    for (size_t i = 0; i < current_sharding_map.size(); ++i)
    {
        current_sharding_topology[current_sharding_map[i]].push_back(i);
    }

    LOG(INFO) << "Before migrate, the average vnodes for each sharding is: " << current_vnode_for_sharding
       << " and sharding topology is : ";
    printSharding(current_sharding_topology);

    std::map<vnodeid_t, std::pair<shardid_t, shardid_t> > migrate_data_list;
    ShardingTopologyT new_sharding_topology;

    std::vector<shardid_t> left_sharding_nodes;
    bool ret = removeSharding(remove_sharding_nodes, left_sharding_nodes,
        new_vnode_for_sharding,
        current_sharding_topology,
        migrate_data_list,
        current_sharding_map);

    if (!ret || left_sharding_nodes.empty())
    {
        LOG(INFO) << "removing nodes are wrong.";
        return false;
    }
    LOG(INFO) << "After migrate, the average vnodes for each sharding will be: " << new_vnode_for_sharding 
       << " and sharding topology will be : ";
    printSharding(current_sharding_topology);

    if(!MasterManagerBase::get()->notifyAllShardingBeginMigrate(getShardidListForSearch()))
    {
        return false;
    }

    ret = doMigrateWork(true, migrate_data_list, left_sharding_nodes,
        map_file, current_sharding_map);

    // allow new write running.
    MasterManagerBase::get()->notifyAllShardingEndMigrate();

    return ret;

}

}

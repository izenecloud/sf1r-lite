#include "IndexTaskService.h"
#include "IndexBundleHelper.h"

#include <common/JobScheduler.h>
#include <aggregator-manager/IndexWorker.h>
#include <node-manager/SearchNodeManager.h>
#include <node-manager/SearchMasterManager.h>
#include <node-manager/sharding/ScdSharder.h>
#include <node-manager/sharding/ShardingStrategy.h>
#include <node-manager/sharding/ScdDispatcher.h>

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
}

IndexTaskService::~IndexTaskService()
{
}

bool IndexTaskService::index(unsigned int numdoc)
{
    bool result = true;

    if (bundleConfig_->isMasterAggregator())
    {
        task_type task = boost::bind(&IndexTaskService::distributedIndex_, this, numdoc);
        JobScheduler::get()->addTask(task, bundleConfig_->collectionName_);
    }
    else
    {
        indexWorker_->index(numdoc, result);
    }

    return result;
}

bool IndexTaskService::index(boost::shared_ptr<DocumentManager>& documentManager)
{
    return indexWorker_->reindex(documentManager);
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

bool IndexTaskService::destroyDocument(const Value& documentValue)
{
    return indexWorker_->destroyDocument(documentValue);
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

std::string IndexTaskService::getScdDir() const
{
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
    SearchMasterManager::get()->registerIndexStatus(bundleConfig_->collectionName_, true);

    bool ret = distributedIndexImpl_(
                    numdoc,
                    bundleConfig_->collectionName_,
                    bundleConfig_->masterIndexSCDPath(),
                    bundleConfig_->indexShardKeys_);

    SearchMasterManager::get()->registerIndexStatus(bundleConfig_->collectionName_, false);

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
   bool ret = true;
   indexAggregator_->distributeRequest(collectionName, "index", numdoc, ret);

   if (ret)
   {
       bfs::path bkDir = bfs::path(masterScdPath) / SCD_BACKUP_DIR;
       bfs::create_directory(bkDir);
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
        LOG(ERROR) << "No shard key!";
        return false;
    }

    // sharding configuration
    ShardingConfig cfg;
    cfg.setShardNum(SearchNodeManager::get()->getShardNum());
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

}

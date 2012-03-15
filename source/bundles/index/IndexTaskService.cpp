#include "IndexTaskService.h"
#include "IndexBundleHelper.h"

#include <common/JobScheduler.h>
#include <aggregator-manager/IndexAggregator.h>
#include <aggregator-manager/IndexWorker.h>

#include <glog/logging.h>

using namespace izenelib::driver;

namespace sf1r
{
IndexTaskService::IndexTaskService(IndexBundleConfiguration* bundleConfig)
: bundleConfig_(bundleConfig)
{
}

IndexTaskService::~IndexTaskService()
{
}

bool IndexTaskService::index(unsigned int numdoc)
{
    if (bundleConfig_->isMasterAggregator())
    {
        task_type task = boost::bind(&IndexTaskService::indexMaster_, this, numdoc);
        JobScheduler::get()->addTask(task);
        return true;
    }
    else
    {
        bool ret;
        return indexWorker_->index(numdoc, ret);
    }
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


bool IndexTaskService::indexMaster_(unsigned int numdoc)
{
    // scd sharding & dispatching
    string scdPath = bundleConfig_->collPath_.getScdPath() + "index/master";
    if (!indexAggregator_->ScdDispatch(
            numdoc,
            bundleConfig_->collectionName_,
            scdPath,
            bundleConfig_->indexShardKeys_))
    {
        return false;
    }

    // backup master

    // distributed indexing request
    LOG(INFO) << "start distributed indexing";
    bool ret = true;
    indexAggregator_->setDebug(true);//xxx
    indexAggregator_->distributeRequest(bundleConfig_->collectionName_, "index", numdoc, ret);
    return true;
}

}

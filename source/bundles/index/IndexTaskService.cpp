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
        task_type task = boost::bind(&IndexTaskService::distributedIndex_, this, numdoc);
        JobScheduler::get()->addTask(task, bundleConfig_->collectionName_);
        return true;
    }

    return indexWorker_->index(numdoc);
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
    return indexAggregator_->distributedIndex(
                numdoc,
                bundleConfig_->collectionName_,
                bundleConfig_->masterIndexSCDPath(),
                bundleConfig_->indexShardKeys_);
}

}

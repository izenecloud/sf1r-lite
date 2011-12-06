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
    // distributed index
    if (bundleConfig_->isSupportByAggregator())
    {
        task_type task = boost::bind(&IndexTaskService::indexMaster_, this, numdoc);
        JobScheduler::get()->addTask(task);
        return true;
    }
    // index on localhost
    else
    {
        bool ret;
        return indexWorker_->index(numdoc, ret);
    }
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
        //xxx pending
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

uint32_t IndexTaskService::getDocNum()
{
    return indexWorker_->getDocNum();
}

std::string IndexTaskService::getScdDir() const
{
    return bundleConfig_->indexSCDPath();
}

}

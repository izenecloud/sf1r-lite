#include "IndexTaskService.h"
#include "IndexBundleHelper.h"

#include <common/JobScheduler.h>
#include <aggregator-manager/IndexAggregator.h>
#include <aggregator-manager/IndexWorker.h>

#include <util/scheduler.h>

#include <glog/logging.h>

using namespace izenelib::driver;

namespace sf1r
{
IndexTaskService::IndexTaskService(IndexBundleConfiguration* bundleConfig)
: bundleConfig_(bundleConfig)
, cronJobName_("IndexTaskService-" + bundleConfig_->collectionName_)
{
    if (cronExpression_.setExpression(bundleConfig_->cronIndexer_))
    {
        bool result = izenelib::util::Scheduler::addJob(
                                        cronJobName_,
                                        5*60*1000,
                                        5*60*1000,
                                        boost::bind(&IndexTaskService::cronJob_, this));
        if (!result)
        {
            LOG(ERROR) << "Failed in izenelib::util::Scheduler::addJob(), cron job name: " << cronJobName_;
        }
    }
}

IndexTaskService::~IndexTaskService()
{
    izenelib::util::Scheduler::removeJob(cronJobName_);
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

uint32_t IndexTaskService::getKeyCount(const std::string& property_name)
{
    return indexWorker_->getKeyCount(property_name);
}

std::string IndexTaskService::getScdDir() const
{
    return bundleConfig_->indexSCDPath();
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

void IndexTaskService::cronJob_()
{
    if (cronExpression_.matches_now())
    {
        indexWorker_->optimizeIndexIdSpace();
    }
}

}

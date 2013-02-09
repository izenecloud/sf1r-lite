#include "product_cron_job_handler.h"

#include <product-manager/product_price_trend.h>
#include <util/scheduler.h>

#include <node-manager/RequestLog.h>
#include <node-manager/DistributeRequestHooker.h>
#include <node-manager/NodeManagerBase.h>
#include <node-manager/MasterManagerBase.h>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

using namespace sf1r;

static const std::string cronJobName = "ProductCronJobHandler";

ProductCronJobHandler::ProductCronJobHandler()
    : wait_seconds_(3600)
    , wait_days_(7)
    , cron_started_(false)
{

}

ProductCronJobHandler::~ProductCronJobHandler()
{
    izenelib::util::Scheduler::removeJob(cronJobName);
}

void ProductCronJobHandler::setParam(uint32_t wait_seconds, uint32_t wait_days)
{
    wait_seconds_ = wait_seconds;
    wait_days_ = wait_days;
}

void ProductCronJobHandler::addCollection(ProductPriceTrend* price_trend)
{
    boost::mutex::scoped_lock lock(mtx_);
    price_trend_list_.push_back(price_trend);
}

void ProductCronJobHandler::runEvents()
{
    boost::unique_lock<boost::mutex> lock(mtx_, boost::try_to_lock);
    if (!lock.owns_lock())
    {
        std::cout << "ProductCronJobHandler locks" << std::endl;
        return;
    }
    std::cout << "[Start doing price trend cron job]" << std::endl;

    //for query recommend
    for (std::vector<ProductPriceTrend *>::const_iterator it = price_trend_list_.begin();
            it!=price_trend_list_.end(); ++it)
    {
        (*it)->CronJob();
    }

    std::cout << "[Finished price trend cron job]" << std::endl;
}

bool ProductCronJobHandler::cronStart(const std::string& cron_job)
{
    if (cron_started_)
    {
        return true;
    }
    std::cout << "ProductCronJobHandler cron starting : " << cron_job << std::endl;
    if (!cron_expression_.setExpression(cron_job))
    {
        return false;
    }
    boost::function<void (int)> task = boost::bind(&ProductCronJobHandler::cronJob_,this, _1);
    izenelib::util::Scheduler::addJob(cronJobName, 60 * 1000, 0, task);
    cron_started_ = true;
    return true;
}

void ProductCronJobHandler::cronJob_(int calltype)
{
    if (cron_expression_.matches_now() || calltype > 0)
    {
        if (calltype == 0)
        {
	    if (NodeManagerBase::get()->isPrimary())
	    {
		MasterManagerBase::get()->pushWriteReq(cronJobName, "cron");
		LOG(INFO) << "push cron job to queue on primary : " << cronJobName;
	    }
	    else
	    {
		LOG(INFO) << "cron job on replica ignored. ";
	    }
	    return;
        }
        if (!DistributeRequestHooker::get()->isValid())
        {
            LOG(INFO) << "cron job ignored : " << __FUNCTION__;
            return;
        }
        CronJobReqLog reqlog;
        if (!DistributeRequestHooker::get()->prepare(Req_CronJob, reqlog))
        {
            LOG(ERROR) << "!!!! prepare log failed while running cron job. : " << __FUNCTION__ << std::endl;
            return;
        }

        runEvents();
	DistributeRequestHooker::get()->processLocalFinished(true);
    }
}

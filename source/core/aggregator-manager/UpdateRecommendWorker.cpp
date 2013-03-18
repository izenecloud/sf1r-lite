#include "UpdateRecommendWorker.h"

#include <node-manager/DistributeRequestHooker.h>
#include <node-manager/RequestLog.h>
#include <util/driver/Request.h>

#include <glog/logging.h>
#include <boost/bind.hpp>

using namespace izenelib::driver;

namespace sf1r
{

UpdateRecommendWorker::UpdateRecommendWorker(
    ItemCFManager& itemCFManager,
    CoVisitManager& coVisitManager
)
    : itemCFManager_(itemCFManager)
    , coVisitManager_(coVisitManager)
{
}

void UpdateRecommendWorker::HookDistributeRequestForUpdateRec(int hooktype, const std::string& reqdata, bool& result)
{
    DistributeRequestHooker::get()->setHook(hooktype, reqdata);
    DistributeRequestHooker::get()->hookCurrentReq(reqdata);
    DistributeRequestHooker::get()->processLocalBegin();
    LOG(INFO) << "got hook request on the UpdateRecommendWorker.";
    result = true;
}

void UpdateRecommendWorker::updatePurchaseMatrix(
    const std::list<itemid_t>& oldItems,
    const std::list<itemid_t>& newItems,
    bool& result
)
{
    bool force_success = DistributeRequestHooker::get()->getHookType() == Request::FromDistribute;
    result = force_success;

    DISTRIBUTE_WRITE_BEGIN_ASYNC;
    DISTRIBUTE_WRITE_CHECK_VALID_RETURN2;

    NoAdditionReqLog reqlog;
    if(!DistributeRequestHooker::get()->prepare(Req_NoAdditionDataReq, reqlog))
    {
        LOG(ERROR) << "prepare failed in " << __FUNCTION__;
        return;
    }

    itemCFManager_.updateMatrix(oldItems, newItems);
    result = true;

    DISTRIBUTE_WRITE_FINISH(true);
}

void UpdateRecommendWorker::updatePurchaseCoVisitMatrix(
    const std::list<itemid_t>& oldItems,
    const std::list<itemid_t>& newItems,
    bool& result
)
{
    bool force_success = DistributeRequestHooker::get()->getHookType() == Request::FromDistribute;
    result = force_success;

    DISTRIBUTE_WRITE_BEGIN_ASYNC;
    DISTRIBUTE_WRITE_CHECK_VALID_RETURN2;

    NoAdditionReqLog reqlog;
    if(!DistributeRequestHooker::get()->prepare(Req_NoAdditionDataReq, reqlog))
    {
        LOG(ERROR) << "prepare failed in " << __FUNCTION__;
        return;
    }
    itemCFManager_.updateVisitMatrix(oldItems, newItems);
    result = true;

    DISTRIBUTE_WRITE_FINISH(true);
}

void UpdateRecommendWorker::buildPurchaseSimMatrix(bool& result)
{
    result = true;
    if (!DistributeRequestHooker::get()->isHooked() ||
        DistributeRequestHooker::get()->getHookType() == Request::FromDistribute)
    {
        jobScheduler_.addTask(boost::bind(&UpdateRecommendWorker::buildPurchaseSimMatrixFunc, this));
    }
    else
    {
        LOG(INFO) << "build Purchase sim matrix on secondary.";
        result = buildPurchaseSimMatrixFunc();
    }
}

bool UpdateRecommendWorker::buildPurchaseSimMatrixFunc()
{
    DISTRIBUTE_WRITE_BEGIN_ASYNC;
    DISTRIBUTE_WRITE_CHECK_VALID_RETURN;

    NoAdditionReqLog reqlog;
    if(!DistributeRequestHooker::get()->prepare(Req_NoAdditionDataReq, reqlog))
    {
        LOG(ERROR) << "prepare failed in " << __FUNCTION__;
        return false;
    }

    itemCFManager_.buildSimMatrix();

    DISTRIBUTE_WRITE_FINISH(true);
    return true;
}

void UpdateRecommendWorker::updateVisitMatrix(
    const std::list<itemid_t>& oldItems,
    const std::list<itemid_t>& newItems,
    bool& result
)
{
    bool force_success = DistributeRequestHooker::get()->getHookType() == Request::FromDistribute;
    result = force_success;

    DISTRIBUTE_WRITE_BEGIN_ASYNC;
    DISTRIBUTE_WRITE_CHECK_VALID_RETURN2;

    NoAdditionReqLog reqlog;
    if(!DistributeRequestHooker::get()->prepare(Req_NoAdditionDataReq, reqlog))
    {
        LOG(ERROR) << "prepare failed in " << __FUNCTION__;
        return;
    }
    coVisitManager_.visit(oldItems, newItems);
    result = true;

    DISTRIBUTE_WRITE_FINISH(true);
}

void UpdateRecommendWorker::flushRecommendMatrix(bool& result)
{
    bool force_success = DistributeRequestHooker::get()->getHookType() == Request::FromDistribute;
    result = force_success;

    DISTRIBUTE_WRITE_BEGIN_ASYNC;
    DISTRIBUTE_WRITE_CHECK_VALID_RETURN2;

    NoAdditionReqLog reqlog;
    if(!DistributeRequestHooker::get()->prepare(Req_NoAdditionDataReq, reqlog))
    {
        LOG(ERROR) << "prepare failed in " << __FUNCTION__;
        return;
    }
    jobScheduler_.addTask(boost::bind(&UpdateRecommendWorker::flushImpl_, this));
    result = true;
    LOG(INFO) << "UpdateRecommendWorker waiting flush ";
    jobScheduler_.waitCurrentFinish();
    LOG(INFO) << "UpdateRecommendWorker wait flush success";

    DISTRIBUTE_WRITE_FINISH(true);
}

void UpdateRecommendWorker::flushImpl_()
{
    coVisitManager_.flush();
    itemCFManager_.flush();

    LOG(INFO) << "flushed [Visit] " << coVisitManager_.matrix();
    LOG(INFO) << "flushed [Purchase] " << itemCFManager_;
}

} // namespace sf1r

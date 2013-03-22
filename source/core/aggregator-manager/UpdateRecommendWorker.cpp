#include "UpdateRecommendWorker.h"

#include <node-manager/DistributeRequestHooker.h>
#include <node-manager/NodeManagerBase.h>
#include <node-manager/DistributeDriver.h>
#include <node-manager/RequestLog.h>
#include <util/driver/Request.h>

#include <glog/logging.h>
#include <boost/bind.hpp>

using namespace izenelib::driver;

namespace sf1r
{

UpdateRecommendWorker::UpdateRecommendWorker(
    const std::string& collection,
    ItemCFManager& itemCFManager,
    CoVisitManager& coVisitManager
)
    : itemCFManager_(itemCFManager)
    , coVisitManager_(coVisitManager)
{
    // set callback used for distribute.
    if (NodeManagerBase::get()->isDistributed())
    {
        DistributeDriver::get()->addCallbackWriteHandler(collection + "_updateVisitMatrix",
            boost::bind(&UpdateRecommendWorker::updateVisitMatrixFunc, this, _1));
        DistributeDriver::get()->addCallbackWriteHandler(collection + "_updatePurchaseMatrix",
            boost::bind(&UpdateRecommendWorker::updatePurchaseMatrixFunc, this, _1));
        DistributeDriver::get()->addCallbackWriteHandler(collection + "_updatePurchaseCoVisitMatrix",
            boost::bind(&UpdateRecommendWorker::updatePurchaseCoVisitMatrixFunc, this, _1));
        DistributeDriver::get()->addCallbackWriteHandler(collection + "_buildPurchaseSimMatrix",
            boost::bind(&UpdateRecommendWorker::buildPurchaseSimMatrixFunc, this, _1));
    }
}

void UpdateRecommendWorker::HookDistributeRequestForUpdateRec(int hooktype, const std::string& reqdata, bool& result)
{
    LOG(INFO) << "callback for new request from shard, packeddata len: " << reqdata.size();
    CommonReqData reqloghead;
    if(!ReqLogMgr::unpackReqLogData(reqdata, reqloghead))
    {
        LOG(ERROR) << "unpack request data failed. data: " << reqdata;
        result = false;
        return;
    }

    DistributeDriver::get()->pushCallbackWrite(reqloghead.req_json_data, reqdata);

    LOG(INFO) << "got hook request on the UpdateRecommendWorker.";
    result = true;
}

bool UpdateRecommendWorker::updatePurchaseMatrixFunc(int calltype)
{
    if (calltype == Request::FromOtherShard)
    {
        LOG(INFO) << "sharding call in " << __FUNCTION__;
    }

    DISTRIBUTE_WRITE_BEGIN;
    DISTRIBUTE_WRITE_CHECK_VALID_RETURN;

    UpdateRecCallbackReqLog reqlog;
    if(!DistributeRequestHooker::get()->prepare(Req_Callback_UpdateRec, reqlog))
    {
        LOG(ERROR) << "prepare failed in " << __FUNCTION__;
        return false;
    }

    itemCFManager_.updateMatrix(reqlog.oldItems, reqlog.newItems);

    DISTRIBUTE_WRITE_FINISH(true);
    return true;
}

void UpdateRecommendWorker::updatePurchaseMatrix(
    const std::list<itemid_t>& oldItems,
    const std::list<itemid_t>& newItems,
    bool& result
)
{
    itemCFManager_.updateMatrix(oldItems, newItems);
    result = true;
}

bool UpdateRecommendWorker::updatePurchaseCoVisitMatrixFunc(int calltype)
{
    if (calltype == Request::FromOtherShard)
    {
        LOG(INFO) << "sharding call in " << __FUNCTION__;
    }

    DISTRIBUTE_WRITE_BEGIN;
    DISTRIBUTE_WRITE_CHECK_VALID_RETURN;

    UpdateRecCallbackReqLog reqlog;
    if(!DistributeRequestHooker::get()->prepare(Req_Callback_UpdateRec, reqlog))
    {
        LOG(ERROR) << "prepare failed in " << __FUNCTION__;
        return false;
    }

    itemCFManager_.updateVisitMatrix(reqlog.oldItems, reqlog.newItems);
    DISTRIBUTE_WRITE_FINISH(true);
    return true;
}

void UpdateRecommendWorker::updatePurchaseCoVisitMatrix(
    const std::list<itemid_t>& oldItems,
    const std::list<itemid_t>& newItems,
    bool& result
)
{
    itemCFManager_.updateVisitMatrix(oldItems, newItems);
    result = true;
}

void UpdateRecommendWorker::buildPurchaseSimMatrix(bool& result)
{
    result = true;
    if (!DistributeRequestHooker::get()->isHooked())
    {
        jobScheduler_.addTask(boost::bind(&ItemCFManager::buildSimMatrix, &itemCFManager_));
    }
    else
    {
        itemCFManager_.buildSimMatrix();
    }
}

bool UpdateRecommendWorker::buildPurchaseSimMatrixFunc(int calltype)
{
    if (calltype == Request::FromOtherShard)
    {
        LOG(INFO) << "sharding call in " << __FUNCTION__;
    }

    DISTRIBUTE_WRITE_BEGIN;
    DISTRIBUTE_WRITE_CHECK_VALID_RETURN;

    BuildPurchaseSimCallbackReqLog reqlog;
    if(!DistributeRequestHooker::get()->prepare(Req_Callback_BuildPurchaseSim, reqlog))
    {
        LOG(ERROR) << "prepare failed in " << __FUNCTION__;
        return false;
    }

    LOG(INFO) << "build Purchase sim matrix on secondary.";
    itemCFManager_.buildSimMatrix();

    DISTRIBUTE_WRITE_FINISH2(true, reqlog);
    return true;
}

bool UpdateRecommendWorker::updateVisitMatrixFunc(int calltype)
{
    if (calltype == Request::FromOtherShard)
    {
        LOG(INFO) << "sharding call in " << __FUNCTION__;
    }

    DISTRIBUTE_WRITE_BEGIN;
    DISTRIBUTE_WRITE_CHECK_VALID_RETURN;

    UpdateRecCallbackReqLog reqlog;
    if(!DistributeRequestHooker::get()->prepare(Req_Callback_UpdateRec, reqlog))
    {
        LOG(ERROR) << "prepare failed in " << __FUNCTION__;
        return false;
    }

    coVisitManager_.visit(reqlog.oldItems, reqlog.newItems);

    DISTRIBUTE_WRITE_FINISH2(true, reqlog);
    return true;
}

void UpdateRecommendWorker::updateVisitMatrix(
    const std::list<itemid_t>& oldItems,
    const std::list<itemid_t>& newItems,
    bool& result
)
{
    coVisitManager_.visit(oldItems, newItems);
    result = true;
}

void UpdateRecommendWorker::flushRecommendMatrix(bool& result)
{
    jobScheduler_.addTask(boost::bind(&UpdateRecommendWorker::flushImpl_, this));
    result = true;
    LOG(INFO) << "UpdateRecommendWorker waiting flush ";
    jobScheduler_.waitCurrentFinish();
    LOG(INFO) << "UpdateRecommendWorker wait flush success";
}

void UpdateRecommendWorker::flushImpl_()
{
    coVisitManager_.flush();
    itemCFManager_.flush();

    LOG(INFO) << "flushed [Visit] " << coVisitManager_.matrix();
    LOG(INFO) << "flushed [Purchase] " << itemCFManager_;
}

} // namespace sf1r

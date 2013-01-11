#include "DistributeRequestHooker.h"
#include <boost/filesystem.hpp>
#include <util/driver/writers/JsonWriter.h>
#include <util/driver/readers/JsonReader.h>
#include <bundles/index/IndexBundleConfiguration.h>
#include <node-manager/SearchNodeManager.h>

#include "RequestLog.h"

using namespace izenelib::driver;
using namespace izenelib;

namespace sf1r
{

DistributeRequestHooker::DistributeRequestHooker()
{
    // init callback for distribute request.

}

void DistributeRequestHooker::hookCurrentReq(const std::string& colname, const CollectionPath& colpath,
    const std::string& reqdata,
    boost::shared_ptr<ReqLogMgr> req_log_mgr)
{
    colname_ = colname;
    colpath_ = colpath;
    current_req_ = reqdata;
    req_log_mgr_ = req_log_mgr;
}

bool DistributeRequestHooker::isHooked()
{
    return !current_req_.empty();
}

bool DistributeRequestHooker::prepare()
{
    if (!isHooked())
        return true;
    if (!req_log_mgr_)
        return false;
    //static JsonReader reader;
    //Value requestValue;
    //if (!reader.read(current_req_, requestValue) || requestValue.type() != Value::kObjectType)
    //{
    //    return false;
    //}
    //Request request;
    //request.assignTmp(requestValue);
    CommonReqData req_common_data;
    req_common_data.req_json_data = current_req_;
    if (!req_log_mgr_->prepareReqLog(req_common_data, SearchNodeManager::get()->isPrimary()))
    {
        return false;
    }
    // backup changeable data first, so that we can rollback if old data corrupt while process the request.
    // Ignore SCD files and any other files that will not change during processing.
    std::string coldata_path = colpath_.getCollectionDataPath();
    std::string querydata_path = colpath_.getQueryDataPath();

    try
    {
        boost::filesystem3::copy(coldata_path, coldata_path + "req-backup-" +
            boost::lexical_cast<std::string>(req_common_data.inc_id));
        boost::filesystem3::copy(querydata_path, querydata_path + "req-backup-" +
            boost::lexical_cast<std::string>(req_common_data.inc_id));
    }
    catch(const std::exception& e)
    {
        // clear state.
        std::cerr << "backup for write request failed. " << e.what() << std::endl;
        abortRequest();
        return false;
    }

    return true;
}

void DistributeRequestHooker::processLocalBegin()
{
    if (!isHooked())
        return;
    LOG(INFO) << "begin process request on worker";
    SearchNodeManager::get()->setNodeState(NodeManagerBase::NODE_STATE_PROCESSING_REQ_RUNNING);
}

void DistributeRequestHooker::processLocalFinished()
{
    if (!isHooked())
        return;
    LOG(INFO) << "begin process request on local worker finished.";
    if (SearchNodeManager::get()->isPrimary())
    {
        process2Replicas();
    }
    else
    {
        SearchNodeManager::get()->setNodeState(NodeManagerBase::NODE_STATE_PROCESSING_REQ_WAIT_PRIMARY);
    }
}

bool DistributeRequestHooker::process2Replicas()
{
    if (!isHooked())
        return true;
    LOG(INFO) << "send request to other replicas.";
    SearchNodeManager::get()->updateWriteReqForReplicas(current_req_);
    return true;
}

bool DistributeRequestHooker::waitReplicasProcessCallback()
{
    if (!isHooked())
        return true;
    LOG(INFO) << "all replicas finished the request.";
    writeLocalLog();
    writeLog2Replicas();
    return true;
}

bool DistributeRequestHooker::waitPrimaryCallback()
{
    if (!isHooked())
        return true;
    LOG(INFO) << "got respond from primary while waiting write log after finished on replica.";
    writeLocalLog();
    return true;
}

void DistributeRequestHooker::abortRequest()
{
    if (!isHooked())
        return;
    LOG(INFO) << "got abort request.";
    CommonReqData prepared_reqdata;
    if (!req_log_mgr_->getPreparedReqLog(prepared_reqdata))
    {
        LOG(ERROR) << "get prepare request log data failed while abort request.";
        return;
    }
    req_log_mgr_->delPreparedReqLog();
    current_req_.clear();
    req_log_mgr_.reset();
    // restore data from backup.
    try
    {
        // rename current and move restore.
    }
    catch(const std::exception& e)
    {
    }
}

bool DistributeRequestHooker::waitReplicasAbortCallback()
{
    if (!isHooked())
        return true;
    LOG(INFO) << "all replicas finished the abort request.";
    return true;
}

bool DistributeRequestHooker::writeLocalLog()
{
    if (!isHooked())
        return true;
    LOG(INFO) << "begin write request log to local.";
    return true;
}

void DistributeRequestHooker::writeLog2Replicas()
{
    if (!isHooked())
        return;
    LOG(INFO) << "begin notify all replicas to write request log to local.";
}

bool DistributeRequestHooker::waitReplicasLogCallback()
{
    if (!isHooked())
        return true;
    LOG(INFO) << "all replicas finished write request log to local.";
    LOG(INFO) << "The request has finally finished both on primary and replicas.";
    finish();
    return true;
}

void DistributeRequestHooker::finish()
{
    // remove backup for current request.
}

}


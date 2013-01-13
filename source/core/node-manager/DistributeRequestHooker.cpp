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

bool DistributeRequestHooker::prepare(ReqLogType type, CommonReqData& prepared_req)
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
    prepared_req.req_json_data = current_req_;
    prepared_req.reqtype = type;
    if (!req_log_mgr_->prepareReqLog(prepared_req, SearchNodeManager::get()->isPrimary()))
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
            boost::lexical_cast<std::string>(prepared_req.inc_id));
        boost::filesystem3::copy(querydata_path, querydata_path + "req-backup-" +
            boost::lexical_cast<std::string>(prepared_req.inc_id));
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
    SearchNodeManager::get()->beginReqProcess();
}

void DistributeRequestHooker::processLocalFinished(const std::string& packed_req_data)
{
    if (!isHooked())
        return;
    LOG(INFO) << "begin process request on local worker finished.";
    current_req_ = packed_req_data;
    SearchNodeManager::get()->finishLocalReqProcess(packed_req_data);
}

bool DistributeRequestHooker::waitReplicasProcessCallback()
{
    if (!isHooked())
        return true;
    LOG(INFO) << "all replicas finished the request. Begin write log";
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
    SearchNodeManager::get()->abortRequest();
}

void DistributeRequestHooker::abortRequestCallback()
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
    bool ret = req_log_mgr_->appendReqData(current_req_);
    if (!ret)
    {
        LOG(ERROR) << "write local log failed. something wrong on this node, must exit.";
        forceExit();
        return false;
    }
    return true;
}

void DistributeRequestHooker::writeLog2Replicas()
{
    if (!isHooked())
        return;
    LOG(INFO) << "begin notify all replicas to write request log to local.";
    SearchNodeManager::get()->notifyWriteReqLog2Replicas();
}

bool DistributeRequestHooker::waitReplicasLogCallback()
{
    if (!isHooked())
        return true;
    LOG(INFO) << "all replicas finished write request log to local.";
    LOG(INFO) << "The request has finally finished both on primary and replicas.";
    writeLocalLog();
    finish();
    return true;
}

void DistributeRequestHooker::finish()
{
    // remove backup if needed for current request.
}

void DistributeRequestHooker::forceExit()
{
    throw -1;
}

}


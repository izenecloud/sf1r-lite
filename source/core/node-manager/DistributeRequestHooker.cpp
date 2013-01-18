#include "DistributeRequestHooker.h"
#include "DistributeDriver.h"
#include "RecoveryChecker.h"
#include <boost/filesystem.hpp>
#include <util/driver/writers/JsonWriter.h>
#include <util/driver/readers/JsonReader.h>
#include <bundles/index/IndexBundleConfiguration.h>
#include <node-manager/SearchNodeManager.h>

#include "RequestLog.h"

using namespace izenelib::driver;
using namespace izenelib;
namespace bfs = boost::filesystem;

namespace sf1r
{

std::set<ReqLogType> DistributeRequestHooker::need_backup_types_;

void DistributeRequestHooker::init()
{
    need_backup_types_.insert(Req_Index);
    need_backup_types_.insert(Req_CreateDoc);
}

DistributeRequestHooker::DistributeRequestHooker()
    :type_(Req_None), hook_type_(0)
{
    // init callback for distribute request.
    SearchNodeManager::get()->setCallback(
        boost::bind(&DistributeRequestHooker::onElectingFinished, this),
        boost::bind(&DistributeRequestHooker::waitReplicasProcessCallback, this),
        boost::bind(&DistributeRequestHooker::waitReplicasLogCallback, this),
        boost::bind(&DistributeRequestHooker::waitPrimaryCallback, this),
        boost::bind(&DistributeRequestHooker::abortRequestCallback, this),
        boost::bind(&DistributeRequestHooker::waitReplicasAbortCallback, this),
        boost::bind(&DistributeRequestHooker::onRequestFromPrimary, this, _1, _2));
}

bool DistributeRequestHooker::isNeedBackup(ReqLogType type)
{
    return need_backup_types_.find(type) != need_backup_types_.end();
}

bool DistributeRequestHooker::isValid()
{
    if (SearchMasterManager::get()->isDistribute() && hook_type_ == 0)
        return false;
    return true;
}

void DistributeRequestHooker::hookCurrentReq(const std::string& colname, const CollectionPath& colpath,
    const std::string& reqdata,
    boost::shared_ptr<ReqLogMgr> req_log_mgr)
{
    colname_ = colname;
    colpath_ = colpath;
    current_req_ = reqdata;
    req_log_mgr_ = req_log_mgr;
    LOG(INFO) << "current request hooked: " << colname_ << "," << current_req_;
}

bool DistributeRequestHooker::onRequestFromPrimary(int type, const std::string& packed_reqdata)
{
    LOG(INFO) << "callback for new request from primary";
    CommonReqData reqloghead;
    if(!ReqLogMgr::unpackReqLogData(packed_reqdata, reqloghead))
    {
        LOG(ERROR) << "unpack request data from primary failed.";
        // return false to abortRequest.
        return false;
    }
    bool ret = DistributeDriver::get()->handleReqFromPrimary(reqloghead.req_json_data, packed_reqdata);
    if (!ret)
    {
        LOG(ERROR) << "send request come from primary failed in replica. " << reqloghead.req_json_data;
        return false;
    }
    LOG(INFO) << "send the request come from primary success in replica.";
    return true;
}

void DistributeRequestHooker::setHook(int calltype, const std::string& addition_data)
{
    LOG(INFO) << "setting hook : " << hook_type_ << ", data:" << primary_addition_;
    hook_type_ = calltype;
    // for request for primary master, the addition_data is the original request json data.
    // for request from primary worker to replicas, the addition_data is original request
    // json data plus the data used for this request.
    primary_addition_ = addition_data;
}

int  DistributeRequestHooker::getHookType()
{
    return hook_type_;
}

bool DistributeRequestHooker::isHooked()
{
    return (hook_type_ > 0) && !current_req_.empty();
}

bool DistributeRequestHooker::prepare(ReqLogType type, CommonReqData& prepared_req)
{
    if (!isHooked())
        return true;
    assert(req_log_mgr_);
    bool isprimary = SearchNodeManager::get()->isPrimary();
    if (isprimary)
        prepared_req.req_json_data = current_req_;
    else
    {
        // get addition data from primary
        CommonReqData reqloghead;
        if(!ReqLogMgr::unpackReqLogData(current_req_, reqloghead))
        {
            LOG(ERROR) << "unpack log data failed while prepare the data from primary.";
            forceExit();
        }
        if (type != (ReqLogType)reqloghead.reqtype)
        {
            LOG(ERROR) << "log type mismatch with primary while prepare the data from primary.";
            LOG(ERROR) << "It may happen when the code is not the same. Must exit.";
            forceExit();
        }
        prepared_req.inc_id = reqloghead.inc_id;
        prepared_req.req_json_data = reqloghead.req_json_data;
        LOG(INFO) << "got create document request from primary, inc_id :" << prepared_req.inc_id;
    }
    prepared_req.reqtype = type;
    type_ = type;
    LOG(INFO) << "begin prepare log ";
    if (!req_log_mgr_->prepareReqLog(prepared_req, isprimary))
    {
        LOG(ERROR) << "prepare request log failed.";
        if (!isprimary)
        {
            assert(false);
            forceExit();
        }
        finish(false);
        return false;
    }
    std::string basepath = colpath_.getBasePath();
    
    if (isNeedBackup(type))
    {
        LOG(INFO) << "begin backup";
        if(!RecoveryChecker::get()->backup())
        {
            LOG(ERROR) << "backup failed. Maybe not enough space.";
            if (!isprimary)
            {
                forceExit();
            }
            finish(false);
            return false;
        }
    }
    // set rollback flag.
    if(!RecoveryChecker::get()->setRollbackFlag(prepared_req.inc_id))
    {
        LOG(ERROR) << "set rollback failed.";
        if (!isprimary)
        {
            forceExit();
        }
        finish(false);
        return false; 
    }
    return true;
}

void DistributeRequestHooker::processLocalBegin()
{
    if (!isHooked())
        return;
    if (hook_type_ == Request::FromLog)
        return;
    LOG(INFO) << "begin process request on worker";
    SearchNodeManager::get()->beginReqProcess();
}

void DistributeRequestHooker::processLocalFinished(bool finishsuccess, const std::string& packed_req_data)
{
    if (!isHooked())
        return;
    LOG(INFO) << "begin process request on local worker finished.";
    current_req_ = packed_req_data;
    if (!finishsuccess)
    {
        LOG(INFO) << "process finished fail.";
        abortRequest();
        return;
    }
    if (hook_type_ == Request::FromLog)
        return;
    SearchNodeManager::get()->finishLocalReqProcess(type_, packed_req_data);
}

void DistributeRequestHooker::waitReplicasProcessCallback()
{
    if (!isHooked())
        return;
    LOG(INFO) << "all replicas finished the request. Begin write log";
}

void DistributeRequestHooker::waitPrimaryCallback()
{
    if (!isHooked())
        return;
    LOG(INFO) << "got respond from primary while waiting write log after finished on replica.";
    writeLocalLog();
}

void DistributeRequestHooker::abortRequest()
{
    if (!isHooked())
        return;
    if (hook_type_ == Request::FromLog)
    {
        LOG(ERROR) << "redo log failed, must exit.";
        forceExit();
        return;
    }
    LOG(INFO) << "abortRequest...";
    SearchNodeManager::get()->abortRequest();
}

void DistributeRequestHooker::abortRequestCallback()
{
    if (!isHooked())
        return;
    LOG(INFO) << "got abort request.";
    finish(false);
}

void DistributeRequestHooker::waitReplicasAbortCallback()
{
    if (!isHooked())
        return;
    LOG(INFO) << "all replicas finished the abort request.";
    finish(false);
}

void DistributeRequestHooker::writeLocalLog()
{
    if (!isHooked())
        return;

    LOG(INFO) << "begin write request log to local.";
    bool ret = req_log_mgr_->appendReqData(current_req_);
    if (!ret)
    {
        LOG(ERROR) << "write local log failed. something wrong on this node, must exit.";
        forceExit();
    }
    finish(true);
}

void DistributeRequestHooker::waitReplicasLogCallback()
{
    if (!isHooked())
        return;
    LOG(INFO) << "all replicas finished write request log to local.";
    writeLocalLog();
}

void DistributeRequestHooker::onElectingFinished()
{
    LOG(INFO) << "an electing has finished. notify ready to master.";
}

void DistributeRequestHooker::finish(bool success)
{
    if (hook_type_ == Request::FromLog && !success)
    {
        LOG(ERROR) << "redo log failed. must exit";
        forceExit();
    }
    try
    {
        if (success)
        {
            LOG(INFO) << "The request has finally finished both on primary and replicas.";
        }
        else
        {
            LOG(INFO) << "The request failed to finish. rollback from backup.";
            // rollback from backup.
            // rename current and move restore.
            // all the file need to be reopened to make effective.
            if (!RecoveryChecker::get()->rollbackLastFail())
            {
                LOG(ERROR) << "failed to rollback! must exit.";
                forceExit();
            }
        }
    }
    catch(const std::exception& e)
    {
        LOG(ERROR) << "failed finish. must exit.";
        forceExit();
    }
    RecoveryChecker::get()->clearRollbackFlag();
    req_log_mgr_->delPreparedReqLog();
    current_req_.clear();
    req_log_mgr_.reset();
    hook_type_ = 0;
    primary_addition_.clear();
}

void DistributeRequestHooker::forceExit()
{
    throw -1;
}

}


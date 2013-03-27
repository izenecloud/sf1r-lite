#include "DistributeRequestHooker.h"
#include "DistributeDriver.h"
#include "RecoveryChecker.h"
#include "DistributeTest.hpp"

#include <boost/filesystem.hpp>
#include <util/driver/writers/JsonWriter.h>
#include <util/driver/readers/JsonReader.h>
#include <util/scheduler.h>
#include <bundles/index/IndexBundleConfiguration.h>
#include <node-manager/NodeManagerBase.h>

#include "RequestLog.h"

using namespace izenelib::driver;
using namespace izenelib;
namespace bfs = boost::filesystem;

namespace sf1r
{

std::set<ReqLogType> DistributeRequestHooker::need_backup_types_;
std::set<std::string> DistributeRequestHooker::async_or_shard_write_types_;

void DistributeRequestHooker::init()
{
    need_backup_types_.insert(Req_NoAdditionData_NeedBackup_Req);
    //need_backup_types_.insert(Req_CronJob);
    need_backup_types_.insert(Req_Index);
    async_or_shard_write_types_.insert("commands_index");
    async_or_shard_write_types_.insert("documents_visit");
    // init callback for distribute request.
    NodeManagerBase::get()->setCallback(
        boost::bind(&DistributeRequestHooker::onElectingFinished, this),
        boost::bind(&DistributeRequestHooker::waitReplicasProcessCallback, this),
        boost::bind(&DistributeRequestHooker::waitReplicasLogCallback, this),
        boost::bind(&DistributeRequestHooker::waitPrimaryCallback, this),
        boost::bind(&DistributeRequestHooker::abortRequestCallback, this),
        boost::bind(&DistributeRequestHooker::waitReplicasAbortCallback, this),
        boost::bind(&DistributeRequestHooker::onRequestFromPrimary, this, _1, _2));
}

DistributeRequestHooker::DistributeRequestHooker()
    :type_(Req_None), hook_type_(0), chain_status_(NoChain), is_replaying_log_(false)
{
}

bool DistributeRequestHooker::isNeedBackup(ReqLogType type)
{
    return need_backup_types_.find(type) != need_backup_types_.end();
}

bool DistributeRequestHooker::isAsyncWriteRequest(const std::string& controller, const std::string& action)
{
    // handle some special write, these write basically need shard or async write request.
    return async_or_shard_write_types_.find(controller + "_" + action) != async_or_shard_write_types_.end();
}

bool DistributeRequestHooker::isValid()
{
    if (NodeManagerBase::get()->isDistributed() && hook_type_ == 0)
        return false;
    return true;
}

void DistributeRequestHooker::setReplayingLog(bool running, CommonReqData& saved_reqlog)
{
    is_replaying_log_ = running;
    // save current data
    if (is_replaying_log_)
    {
        saved_current_req_ = current_req_;
        saved_type_ = type_;
        saved_hook_type_ = hook_type_;
        saved_chain_status_ = chain_status_;
        clearHook(true);
    }
    else
    {
        // restore saved data.
        current_req_ = saved_current_req_;
        type_ = saved_type_;
        hook_type_ = saved_hook_type_;
        chain_status_ = saved_chain_status_;
        
        req_log_mgr_ = RecoveryChecker::get()->getReqLogMgr();
    }
}

void DistributeRequestHooker::hookCurrentReq(const std::string& reqdata)
{
    if (req_log_mgr_)
    {
        // for the request that will shard to different node, 
        // it is normal to be hook twice. 
        // If no sharding it should be some wrong if hook again.
        LOG(INFO) << "hooked request again!!";
    }
    current_req_ = reqdata;
    req_log_mgr_ = RecoveryChecker::get()->getReqLogMgr();
    LOG(INFO) << "current request hooked, data len: " << current_req_.size() << ", data:" << current_req_;
}

bool DistributeRequestHooker::onRequestFromPrimary(int type, const std::string& packed_reqdata)
{
    TEST_FALSE_RETURN(FalseReturn_At_UnPack);
    LOG(INFO) << "callback for new request from primary, packeddata len: " << packed_reqdata.size();
    CommonReqData reqloghead;
    if(!ReqLogMgr::unpackReqLogData(packed_reqdata, reqloghead))
    {
        LOG(ERROR) << "unpack request data from primary failed. data: " << packed_reqdata;
        // return false to abortRequest.
        return false;
    }
    bool ret = true;
    ret = DistributeDriver::get()->handleReqFromPrimary(reqloghead.reqtype, reqloghead.req_json_data, packed_reqdata);
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
    hook_type_ = calltype;
    // for request for primary master, the addition_data is the original request json data.
    // for request from primary worker to replicas, the addition_data is original request
    // json data plus the data used for this request.
    current_req_ = addition_data;
    LOG(INFO) << "setting hook : " << hook_type_ << ", data:" << current_req_;
    DistributeTestSuit::loadTestConf();
}

int  DistributeRequestHooker::getHookType()
{
    return hook_type_;
}

bool DistributeRequestHooker::isHooked()
{
    return (hook_type_ > 0) && !current_req_.empty();
}

bool DistributeRequestHooker::isRunningPrimary()
{
    return !isHooked() || hook_type_ == Request::FromDistribute || hook_type_ == Request::FromOtherShard;
}

bool DistributeRequestHooker::readPrevChainData(CommonReqData& reqlogdata)
{
    if (!isHooked())
        return true;
    if (current_req_.empty())
        return true;
    if (!req_log_mgr_)
        return true;
    if (!is_replaying_log_)
    {
        CommonReqData prepared;
        if(!req_log_mgr_->getPreparedReqLog(prepared))
        {
            LOG(WARNING) << "try to read chain data before prepared!!!";
            return true;
        }
    }
    if (!ReqLogMgr::unpackReqLogData(current_req_, reqlogdata))
    {
        LOG(ERROR) << "unpack log data error while read from chain data.";
        return false;
    }
    return true;
}

bool DistributeRequestHooker::prepare(ReqLogType type, CommonReqData& prepared_req)
{
    if (!isHooked())
        return true;
    assert(req_log_mgr_);
    bool isprimary = (hook_type_ == Request::FromDistribute || hook_type_ == Request::FromOtherShard);
    if (chain_status_ == ChainStop)
    {
        LOG(WARNING) << "The request has been aborted while prepare";
        return false;
    }
    if (isprimary && hook_type_ == Request::FromDistribute)
    {
        if (chain_status_ == ChainBegin || chain_status_ == NoChain)
            prepared_req.req_json_data = current_req_;
        else if (type != type_)
        {
            LOG(ERROR) << "!!!!! The request type data is not matched during the chain request." <<
                "before: " << type_ << ", after :" << type;
        }
    }
    else
    {
        DistributeTestSuit::testFail(ReplicaFail_At_UnpackPrimaryReq);
        // get addition data from primary
        if(!ReqLogMgr::unpackReqLogData(current_req_, prepared_req))
        {
            LOG(ERROR) << "unpack log data failed while prepare the data.";
            forceExit();
        }
        if (type != (ReqLogType)prepared_req.reqtype)
        {
            LOG(ERROR) << "log type mismatch with primary while prepare the data.";
            LOG(ERROR) << "It may happen when the code is not the same. Must exit.";
            forceExit();
        }
        LOG(INFO) << "got hooked write request, inc_id :" << prepared_req.inc_id;
    }
    prepared_req.reqtype = type;
    type_ = type;
    LOG(INFO) << "begin prepare log ";
    if (chain_status_ == NoChain || chain_status_ == ChainBegin)
    {
        if (!is_replaying_log_ && !req_log_mgr_->prepareReqLog(prepared_req, isprimary))
        {
            LOG(ERROR) << "prepare request log failed.";
            if (!isprimary)
            {
                assert(false);
                forceExit();
            }
            processFinishedBeforePrepare(false);
            return false;
        }
    }
    
    if (isprimary)
    {
        bool needpack = true;
        if (type == Req_NoAdditionData_NeedBackup_Req || type == Req_NoAdditionDataReq ||
            type == Req_NoAdditionDataNoRollback)
        {
            if (chain_status_ != NoChain && chain_status_ != ChainBegin)
                needpack = false;
        }
        if (needpack)
        {
            // save primary prepared data to current_req_ and 
            // after primary finished, send it to replica.
            ReqLogMgr::packReqLogData(prepared_req, current_req_);
        }
    }

    if (chain_status_ != NoChain && chain_status_ != ChainBegin)
    {
        // only prepare at the begin of chain request.
        LOG(INFO) << "no need prepare for middle of request chain.";
        return true;
    }

    if (is_replaying_log_)
    {
        // during replay log , no need backup and set rollback.
        return true;
    }

    if (isNeedBackup(type) || (prepared_req.inc_id % 10000 == 0) || !RecoveryChecker::get()->hasAnyBackup())
    {
        LOG(INFO) << "begin backup";
        if(!RecoveryChecker::get()->backup())
        {
            LOG(ERROR) << "backup failed. Maybe not enough space.";
            if (!isprimary)
            {
                forceExit();
            }
            return false;
        }
        //if (hook_type_ != Request::FromLog)
        //    NodeManagerBase::get()->setSlowWriting();
    }
    // set rollback flag.
    if(type != Req_NoAdditionDataNoRollback && !RecoveryChecker::get()->setRollbackFlag(prepared_req.inc_id))
    {
        LOG(ERROR) << "set rollback failed.";
        if (!isprimary)
        {
            forceExit();
        }
        return false; 
    }
    if (isprimary)
        DistributeTestSuit::testFail(PrimaryFail_At_PrepareFinished);
    else
        DistributeTestSuit::testFail(ReplicaFail_At_PrepareFinished);
    return true;
}

void DistributeRequestHooker::processLocalBegin()
{
    if (!isHooked())
        return;
    // on replica , the begin notify will be sent to primary before callback.
    if (hook_type_ == Request::FromLog || hook_type_ == Request::FromPrimaryWorker)
        return;
    if (chain_status_ != NoChain && chain_status_ != ChainBegin)
    {
        // only need change node state at the begin of chain request.
        LOG(INFO) << "one of chain request begin local";
        return;
    }
    LOG(INFO) << "begin process request on worker";
    NodeManagerBase::get()->beginReqProcess();
}

bool DistributeRequestHooker::processFinishedBeforePrepare(bool finishsuccess)
{
    if (!isHooked())
        return false;
    if (!finishsuccess && hook_type_ == Request::FromLog)
    {
        LOG(ERROR) << "redo log failed finished, must exit.";
        forceExit();
    }
    static CommonReqData reqlog;
    if (!is_replaying_log_ && !req_log_mgr_->getPreparedReqLog(reqlog))
    {
        if (hook_type_ == Request::FromDistribute || hook_type_ == Request::FromOtherShard)
        {
            LOG(INFO) << "primary end request before prepared, request ignored.";
            clearHook(true);
            NodeManagerBase::get()->notifyMasterReadyForNew();
            return true;
        }
        else
        {
            LOG(INFO) << "replica end request before prepared, must exit.";
            forceExit();
        }
    }
    return false;
}

void DistributeRequestHooker::updateReqLogData(CommonReqData& updated_preparedata)
{
    // if the prepared data has changed during processing, 
    // we need update the current_req_ before send it to replicas. 
    ReqLogMgr::packReqLogData(updated_preparedata, current_req_);
}

void DistributeRequestHooker::processLocalFinished(bool finishsuccess)
{
    if (!isHooked())
        return;
    //current_req_ = packed_req_data;
    TEST_FALSE_RET(FalseReturn_At_LocalFinished, finishsuccess)
    if (processFinishedBeforePrepare(finishsuccess))
        return;
    if (!finishsuccess)
    {
        LOG(INFO) << "process finished fail.";
        abortRequest();
        return;
    }

    if (chain_status_ != NoChain && chain_status_ != ChainEnd)
    {
        LOG(INFO) << "request chain finished one of chain that not end.";
        return;
    }

    LOG(INFO) << "process request on local worker finished.";
    if (hook_type_ == Request::FromLog)
    {
        writeLocalLog();
        return;
    }
    // need set rollback flag after primary finished, even no rollback type.
    if(type_ == Req_NoAdditionDataNoRollback)
    {
	    CommonReqData reqlog;
	    req_log_mgr_->getPreparedReqLog(reqlog);
	    if(!RecoveryChecker::get()->setRollbackFlag(reqlog.inc_id))
	    {
		    LOG(ERROR) << "set rollback failed after finish local.this node need restore by human.";
		    forceExit();
	    }
    }
 
    LOG(INFO) << "send packed request data len from local. len: " << current_req_.size();
    NodeManagerBase::get()->finishLocalReqProcess(type_, current_req_);
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
    if (chain_status_ == ChainStop)
    {
        LOG(INFO) << "request already in abortting.";
        return;
    }
    chain_status_ = ChainStop;
    LOG(INFO) << "abortRequest...";
    NodeManagerBase::get()->abortRequest();
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

    assert(chain_status_ == ChainEnd || chain_status_ == NoChain);
    LOG(INFO) << "begin write request log to local.";

    if (!is_replaying_log_)
    {
        bool ret = req_log_mgr_->appendReqData(current_req_);
        if (!ret)
        {
            LOG(ERROR) << "write local log failed. something wrong on this node, must exit.";
            forceExit();
        }
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
    assert(chain_status_ == ChainEnd || chain_status_ == NoChain);
    if (hook_type_ == Request::FromLog && !success)
    {
        LOG(ERROR) << "redo log failed. must exit";
        forceExit();
    }

    CommonReqData reqlog;
    if (!is_replaying_log_)
    {
        req_log_mgr_->getPreparedReqLog(reqlog);
        req_log_mgr_->delPreparedReqLog();
    }
    ReqLogType type = type_;
    clearHook(true);
    if (is_replaying_log_)
        return;
    if (success)
    {
        LOG(INFO) << "The request has finally finished both on primary and replicas.";
        DistributeTestSuit::updateMemoryState("Last_Success_Request", reqlog.inc_id);
        RecoveryChecker::get()->clearRollbackFlag();
        if (isNeedBackup(type))
        {
            LOG(INFO) << "begin backup after finished.";
            if(!RecoveryChecker::get()->backup())
            {
                LOG(ERROR) << "backup failed. Maybe not enough space.";
                forceExit();
            }
        }
    }
    else
    {
        DistributeTestSuit::updateMemoryState("Last_Failed_Request", reqlog.inc_id);
        LOG(INFO) << "The request failed to finish. rollback from backup.";
        // rollback from backup.
        // all the file need to be reopened to make effective.
        if (!RecoveryChecker::get()->rollbackLastFail())
        {
            LOG(ERROR) << "failed to rollback ! must exit.";
            forceExit();
        }
    }

    //if ( (reqlog.inc_id % 10 == 0) && !RecoveryChecker::get()->checkDataConsistent() )
    //{
    //    LOG(ERROR) << "!!!!! finished request with not consistent data: ";
    //}

    LOG(INFO) << DistributeTestSuit::getStatusReport();
}

void DistributeRequestHooker::clearHook(bool force)
{
    static CommonReqData reqlog;
    if (!force && req_log_mgr_->getPreparedReqLog(reqlog))
    {
        // if prepared , we will clear hook after finish .
        return;
    }
    current_req_.clear();
    req_log_mgr_.reset();
    hook_type_ = 0;
    chain_status_ = NoChain;
    LOG(INFO) << "request hook cleard.";
}

void DistributeRequestHooker::forceExit()
{
    RecoveryChecker::forceExit("force exit in DistributeRequestHooker");
}

DistributeWriteGuard::DistributeWriteGuard(bool async)
    : result_setted_(false), async_(async)
{
    // FromLog and FromPrimaryWorker can not be async.
    if (DistributeRequestHooker::get()->getHookType() == Request::FromLog ||
        DistributeRequestHooker::get()->getHookType() == Request::FromPrimaryWorker)
        async_ = false;
}

DistributeWriteGuard::~DistributeWriteGuard()
{
    if (!result_setted_ && async_)
        DistributeRequestHooker::get()->processLocalFinished(false);
}

bool DistributeWriteGuard::isValid()
{
    return DistributeRequestHooker::get()->isValid();
}

void DistributeWriteGuard::setResult()
{
    result_setted_ = true;
}

void DistributeWriteGuard::setResult(bool result)
{
    result_setted_ = true;
    if (async_)
        DistributeRequestHooker::get()->processLocalFinished(result);
}

void DistributeWriteGuard::setResult(bool result, CommonReqData& updated_preparedata)
{
    result_setted_ = true;
    DistributeRequestHooker::get()->updateReqLogData(updated_preparedata);
    if (async_)
    {
        DistributeRequestHooker::get()->processLocalFinished(result);
    }
}

}


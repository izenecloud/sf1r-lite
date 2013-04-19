#ifndef SF1R_DISTRIBUTE_REQUEST_HOOKER_H
#define SF1R_DISTRIBUTE_REQUEST_HOOKER_H

#include "RequestLog.h"
#include <string>
#include <configuration-manager/CollectionPath.h>
#include <util/singleton.h>
#include <set>

namespace sf1r
{
class ReqLogMgr;
class DistributeRequestHooker
{
public:
    static DistributeRequestHooker* get()
    {
        return izenelib::util::Singleton<DistributeRequestHooker>::get();
    }

    enum ChainStatus
    {
        NoChain = 0,
        ChainBegin,
        ChainMiddle,
        ChainEnd,
        ChainStop,
        Unknown
    };

    DistributeRequestHooker();
    void init();
    bool isValid();
    void hookCurrentReq(const std::string& reqdata);
    bool isHooked();
    void setHook(int calltype, const std::string& addition_data);
    int  getHookType();
    bool isRunningPrimary();

    inline bool setChainStatus(ChainStatus status)
    {
        if (!isHooked())
            return true;
        if (chain_status_ > status)
        {
            // chain request can only handle from begin to end.
            // you can not set status to an older status. 
            return false;
        }
        chain_status_ = status;
        return true;
    }
    ChainStatus getChainStatus()
    {
        return chain_status_;
    }
    bool readPrevChainData(CommonReqData& reqlogdata);

    const std::string& getAdditionData()
    {
        return current_req_;
    }

    bool prepare(ReqLogType type, CommonReqData& prepared_req);
    void processLocalBegin();
    void processLocalFinished(bool finishsuccess);
    void updateReqLogData(CommonReqData& updated_preparedata);
    bool processFinishedBeforePrepare(bool finishsuccess);
    bool onRequestFromPrimary(int type, const std::string& packed_reqdata);

    void waitReplicasProcessCallback();
    void waitPrimaryCallback();

    void abortRequest();
    void abortRequestCallback();
    void waitReplicasAbortCallback();

    void writeLocalLog();
    void waitReplicasLogCallback();

    void onElectingFinished();
    static bool isAsyncWriteRequest(const std::string& controller, const std::string& action);

    void setReplayingLog(bool running, CommonReqData& saved_reqlog);

private:
    static bool isNeedBackup(ReqLogType type);
    void finish(bool success);
    void forceExit();
    void clearHook(bool force = false);

    //std::string colname_;
    //CollectionPath colpath_;
    // for primary worker, this is raw json request data.
    // for replica worker, this is packed request data with addition data from primary.
    std::string current_req_;
    ReqLogType type_;
    boost::shared_ptr<ReqLogMgr> req_log_mgr_;
    int hook_type_;
    ChainStatus chain_status_;
    bool is_replaying_log_;
    // saved used for replaying log 
    std::string saved_current_req_;
    ReqLogType saved_type_;
    int saved_hook_type_;
    ChainStatus saved_chain_status_;
    uint32_t last_backup_id_;

    static std::set<ReqLogType> need_backup_types_;
    static std::set<std::string> async_or_shard_write_types_;

};

class DistributeWriteGuard
{
public:
    DistributeWriteGuard(bool async = false);
    ~DistributeWriteGuard();
    void setResult();
    void setResult(bool result);
    void setResult(bool result, CommonReqData& reqlog);
    bool isValid();
private:
    bool result_setted_;
    bool async_;
};

#define DISTRIBUTE_WRITE_BEGIN DistributeWriteGuard distribute_write_guard;
#define DISTRIBUTE_WRITE_BEGIN_ASYNC DistributeWriteGuard distribute_write_guard(true);
#define DISTRIBUTE_WRITE_CHECK_VALID_RETURN  if (!distribute_write_guard.isValid()) { LOG(ERROR) << __FUNCTION__ << " call invalid."; return false; }
#define DISTRIBUTE_WRITE_CHECK_VALID_RETURN2  if (!distribute_write_guard.isValid()) { LOG(ERROR) << __FUNCTION__ << " call invalid."; return; }
#define DISTRIBUTE_WRITE_FINISH(ret)  distribute_write_guard.setResult(ret);
#define DISTRIBUTE_WRITE_FINISH2(ret, reqlog)  distribute_write_guard.setResult(ret, reqlog);
#define DISTRIBUTE_WRITE_FINISH3  distribute_write_guard.setResult();

}

#endif

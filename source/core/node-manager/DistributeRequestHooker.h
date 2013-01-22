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
    static void init();
    static DistributeRequestHooker* get()
    {
        return izenelib::util::Singleton<DistributeRequestHooker>::get();
    }


    DistributeRequestHooker();
    bool isValid();
    void hookCurrentReq(const std::string& colname, const CollectionPath& colpath,
        const std::string& reqdata, boost::shared_ptr<ReqLogMgr> req_log_mgr);
    bool isHooked();
    void setHook(int calltype, const std::string& addition_data);
    int  getHookType();
    void clearHook(bool force = false);
    const std::string& getAdditionData()
    {
        return primary_addition_;
    }

    bool prepare(ReqLogType type, CommonReqData& prepared_req);
    void processLocalBegin();
    void processLocalFinished(bool finishsuccess);
    bool onRequestFromPrimary(int type, const std::string& packed_reqdata);

    void waitReplicasProcessCallback();
    void waitPrimaryCallback();

    void abortRequest();
    void abortRequestCallback();
    void waitReplicasAbortCallback();

    void writeLocalLog();
    void waitReplicasLogCallback();

    void onElectingFinished();

private:
    static bool isNeedBackup(ReqLogType type);
    void finish(bool success);
    void forceExit();

    //std::string colname_;
    //CollectionPath colpath_;
    // for primary worker, this is raw json request data.
    // for replica worker, this is packed request data with addition data from primary.
    std::string current_req_;
    std::string primary_addition_;
    ReqLogType type_;
    boost::shared_ptr<ReqLogMgr> req_log_mgr_;
    int hook_type_;
    static std::set<ReqLogType> need_backup_types_;

};

}

#endif

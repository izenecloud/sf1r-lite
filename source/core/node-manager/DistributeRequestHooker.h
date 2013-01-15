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
    static std::string getRollbackFile(const CollectionPath& colpath, uint32_t inc_id);
    static bool getLastBackup(const CollectionPath& colpath, std::string& backuppath, uint32_t& backup_inc_id);
    static DistributeRequestHooker* get()
    {
        return izenelib::util::Singleton<DistributeRequestHooker>::get();
    }


    DistributeRequestHooker();
    void hookCurrentReq(const std::string& colname, const CollectionPath& colpath,
        const std::string& reqdata, boost::shared_ptr<ReqLogMgr> req_log_mgr);
    bool isHooked();
    void setHook(int calltype, const std::string& addition_data);
    int  getHookType();
    const std::string& getAdditionData()
    {
        return primary_addition_;
    }

    bool prepare(ReqLogType type, CommonReqData& prepared_req);
    void processLocalBegin();
    void processLocalFinished(bool finishsuccess, const std::string& packed_reqdata);
    void onRequestFromPrimary(int type, const std::string& packed_reqdata);

    void waitReplicasProcessCallback();
    void waitPrimaryCallback();

    void abortRequest();
    void abortRequestCallback();
    void waitReplicasAbortCallback();

    void writeLocalLog();
    void waitReplicasLogCallback();

    void onElectingFinished();
    void finish(bool success, const CommonReqData& prepared_req);

private:
    static void getBackupList(const CollectionPath& colpath, std::vector<uint32_t>& backup_req_incids);
    static bool isNeedBackup(ReqLogType type);

    void cleanUnnessesaryBackup();
    bool setRollbackFlag(uint32_t inc_id);
    void clearRollbackFlag(uint32_t inc_id);
    void forceExit();

    std::string colname_;
    CollectionPath colpath_;
    std::string current_req_;
    std::string primary_addition_;
    ReqLogType type_;
    boost::shared_ptr<ReqLogMgr> req_log_mgr_;
    int hook_type_;
    static std::set<ReqLogType> need_backup_types_;

};

}

#endif

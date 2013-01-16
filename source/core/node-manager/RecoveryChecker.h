#ifndef SF1R_NODEMANAGER_RECOVERCHECKER_H
#define SF1R_NODEMANAGER_RECOVERCHECKER_H

#include "RequestLog.h"
#include <string>
#include <configuration-manager/CollectionPath.h>
#include <util/singleton.h>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

namespace sf1r
{

class ReqLogMgr;
class SF1Config;
class RecoveryChecker
{
public:
    typedef boost::function<bool(const std::string&, const std::string&)> StartColCBFuncT;
    typedef boost::function<bool(const std::string&)> StopColCBFuncT;
    static RecoveryChecker* get()
    {
        return ::izenelib::util::Singleton<RecoveryChecker>::get();
    }
    //RecoveryChecker(const CollectionPath& colpath);
    bool backup();
    bool setRollbackFlag(uint32_t inc_id);
    void clearRollbackFlag();
    bool rollbackLastFail();

    void setRestartCallback(StartColCBFuncT start_col, StopColCBFuncT stop_col, StopColCBFuncT stop_clean_col)
    {
        start_col_ = start_col;
        stop_col_ = stop_col;
        stop_clean_col_ = stop_clean_col;
    }
    void init(const std::string& workdir);
    void updateCollection(const SF1Config& sf1_config);
    boost::shared_ptr<ReqLogMgr> getReqLogMgr()
    {
        return reqlog_mgr_;
    }
    void onRecoverCallback();
    void onRecoverWaitPrimaryCallback();
    void onRecoverWaitReplicasCallback();
    void syncToNewestReqLog();
    void wait();
private:
    static bool getLastBackup(const std::string& backup_basepath, std::string& backuppath, uint32_t& backup_inc_id);
    static void getBackupList(const std::string& backup_basepath, std::vector<uint32_t>& backup_req_incids);
    static void cleanUnnessesaryBackup(const std::string& backup_basepath);
    StartColCBFuncT start_col_;
    StopColCBFuncT stop_col_;
    StopColCBFuncT stop_clean_col_;
    std::string backup_basepath_;
    std::string request_log_basepath_;
    std::string redo_log_basepath_;
    std::string rollback_file_;
    boost::shared_ptr<ReqLogMgr> reqlog_mgr_;
    typedef std::map<std::string, std::pair<CollectionPath, std::string> > CollInfoMapT;
    CollInfoMapT all_col_info_;
};

}

#endif

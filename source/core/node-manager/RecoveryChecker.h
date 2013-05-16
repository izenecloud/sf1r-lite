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
    typedef boost::function<bool(const std::string&, const std::string&, bool)> StartColCBFuncT;
    typedef boost::function<bool(const std::string&, bool)> StopColCBFuncT;
    typedef boost::function<void(const std::string&)> FlushColCBFuncT;

    static RecoveryChecker* get()
    {
        return ::izenelib::util::Singleton<RecoveryChecker>::get();
    }

    static void forceExit(const std::string& err = "");
    static void clearForceExitFlag();
    static bool isLastNormalExit();

    //RecoveryChecker(const CollectionPath& colpath);
    bool backup(bool force_remove = true);
    bool setRollbackFlag(uint32_t inc_id);
    void clearRollbackFlag();
    bool rollbackLastFail(bool starting_up = false);
    bool checkAndRestoreBackupFile(const CollectionPath& colpath);
    bool checkDataConsistent();
    bool hasAnyBackup(uint32_t& last_backup_id);
    void flushAllData();

    void setRestartCallback(StartColCBFuncT start_col, StopColCBFuncT stop_col)
    {
        start_col_ = start_col;
        stop_col_ = stop_col;
    }

    void setColCallback(FlushColCBFuncT flush_cb)
    {
        flush_col_ = flush_cb;
    }

    void init(const std::string& conf_dir, const std::string& workdir);
    void addCollection(const std::string& colname, const CollectionPath& colpath, const std::string& configfile);
    void removeCollection(const std::string& colname);
    bool getCollPath(const std::string& colname, CollectionPath& colpath);
    void getCollList(std::vector<std::string>& coll_list);

    //void updateCollection(const SF1Config& sf1_config);
    boost::shared_ptr<ReqLogMgr> getReqLogMgr()
    {
        return reqlog_mgr_;
    }
    void onRecoverCallback(bool startup = true);
    void onRecoverWaitPrimaryCallback();
    void onRecoverWaitReplicasCallback();
    bool onRecoverCheckLog(bool is_primary);

    bool removeConfigFromAPI(const std::string& coll);
    bool updateConfigFromAPI(const std::string& coll, bool is_primary,
        const std::string& config_file, std::map<std::string, std::string>& config_file_list);

    void replayLog(bool is_primary, const std::string& from_col,
        const std::string& to_col, std::vector<uint32_t>& replayed_id_list);

    void checkDataConsistent(const std::string& coll, std::string& errinfo);

private:
    typedef std::map<std::string, std::pair<CollectionPath, std::string> > CollInfoMapT;
    static void setForceExitFlag();
    bool isNeedRollback(bool starting_up);
    bool backupColl(const CollectionPath& colpath, const bfs::path& dest_path);
    void syncToNewestReqLog();
    void syncSCDFiles();
    bool redoLog(ReqLogMgr* redolog, uint32_t start_id, uint32_t end_id);
    std::map<std::string, std::string> handleConfigUpdate();

    bool checkIfLogForward(bool is_primary);

    bool handleConfigUpdateForColl(const std::string& coll, bool delete_conf,
        std::map<std::string, std::string>& config_file_list);

    StartColCBFuncT start_col_;
    StopColCBFuncT stop_col_;
    FlushColCBFuncT flush_col_;
    std::string backup_basepath_;
    std::string request_log_basepath_;
    std::string redo_log_basepath_;
    std::string rollback_file_;
    std::string last_conf_file_;
    std::string configDir_;
    bool need_backup_;
    boost::shared_ptr<ReqLogMgr> reqlog_mgr_;
    CollInfoMapT all_col_info_;
    boost::mutex mutex_;
};

}

#endif

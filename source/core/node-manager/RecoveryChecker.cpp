#include "RecoveryChecker.h"
#include "DistributeFileSyncMgr.h"
#include "DistributeDriver.h"
#include "RequestLog.h"
#include "NodeManagerBase.h"
#include "DistributeTest.hpp"

#include <glog/logging.h>
#include <boost/filesystem.hpp>

#define MAX_BACKUP_NUM 3
namespace bfs = boost::filesystem;

namespace sf1r
{

class CopyGuard
{
public:
    CopyGuard(const std::string& guard_dir)
        :guard_file_(guard_dir + "/" + getGuardFileName())
    {
        // touch a flag
        std::ofstream ofs(guard_file_.c_str());
        ofs << std::flush;
        ofs.close();
    }
    ~CopyGuard()
    {
        // clear a flag
        if (bfs::exists(guard_file_))
        {
            bfs::remove(guard_file_);
        }
    }
    static bool isDirCopyOK(const std::string& dir)
    {
        return !bfs::exists(dir + "/" + getGuardFileName());
    }
    static void safe_remove_all(const std::string& dir)
    {
        CopyGuard guard(dir);
        // remove other files first and then remove flag.
        static bfs::directory_iterator end_it = bfs::directory_iterator();
        bfs::directory_iterator dir_it = bfs::directory_iterator(dir);
        while(dir_it != end_it)
        {
            bfs::path current(dir_it->path());
            if (bfs::is_regular_file(current) &&
                current.filename().string() == getGuardFileName())
            {
                ++dir_it;
                continue;
            }
            DistributeTestSuit::testFail(Fail_At_CopyRemove_File);
            bfs::remove_all(current);
            ++dir_it;
        }
        bfs::remove(dir + "/" + getGuardFileName());
        bfs::remove_all(dir);
    }

    static std::string getGuardFileName()
    {
        static std::string guard_flag("copy_guard_file.flag");
        return guard_flag;
    }

private:
    std::string guard_file_;
};

static void getBackupList(const bfs::path& backup_basepath, std::vector<uint32_t>& backup_req_incids)
{
    if (!bfs::exists(backup_basepath))
        return;
    // only keep the lastest 3 backups.
    static const bfs::directory_iterator end_dir = bfs::directory_iterator();
    bfs::directory_iterator backup_dirs_it = bfs::directory_iterator(backup_basepath);
    while(backup_dirs_it != end_dir)
    {
        if (!bfs::is_directory(*backup_dirs_it))
        {
            ++backup_dirs_it;
            continue;
        }
        try
        {
            uint32_t inc_id = boost::lexical_cast<uint32_t>((*backup_dirs_it).path().filename().string());
            backup_req_incids.push_back(inc_id);
        }
        catch(const boost::bad_lexical_cast& e)
        {
            LOG(INFO) << "not a backup dir : " << (*backup_dirs_it).path().filename();
        }
        ++backup_dirs_it;
    }
    std::sort(backup_req_incids.begin(), backup_req_incids.end(), std::greater<uint32_t>());
}

static void cleanUnnessesaryBackup(const bfs::path& backup_basepath)
{
    // only keep the lastest 3 backups.
    std::vector<uint32_t> backup_req_incids;
    getBackupList(backup_basepath, backup_req_incids);
    int keeped = 0;
    for (size_t i = 0; i < backup_req_incids.size(); ++i)
    {
        if (backup_req_incids[i] == 0)
        {
            // keep the first backup.
            continue;
        }
        bfs::path backup_dir = bfs::path(backup_basepath)/bfs::path(boost::lexical_cast<std::string>(backup_req_incids[i]));
        if (keeped > MAX_BACKUP_NUM)
            CopyGuard::safe_remove_all(backup_dir.string());
        else
        {
            if (CopyGuard::isDirCopyOK(backup_dir.string()))
                ++keeped;
            else 
            {
                LOG(WARNING) << "a corrupted directory removed " << backup_dir;
                CopyGuard::safe_remove_all(backup_dir.string());
            }
        }
    }
}

static bool getLastBackup(const bfs::path& backup_basepath, std::string& backup_path, uint32_t& backup_inc_id)
{
    std::vector<uint32_t> backup_req_incids;
    getBackupList(backup_basepath, backup_req_incids);
    if (backup_req_incids.empty())
        return false;
    for (size_t i = 0; i < backup_req_incids.size(); ++i)
    {
        backup_inc_id = backup_req_incids[i];
        backup_path = (backup_basepath/bfs::path(boost::lexical_cast<std::string>(backup_inc_id))).string() + "/";
        if (CopyGuard::isDirCopyOK(backup_path))
            return true;
        else 
        {
            LOG(WARNING) << "a corrupted directory removed " << backup_path;
            CopyGuard::safe_remove_all(backup_path);
        }
    }
    return false;
}

static void copyDir(bfs::path src, bfs::path dest)
{
    if( !bfs::exists(src) ||
        !bfs::is_directory(src) )
    {
        RecoveryChecker::forceExit("Source directory " + src.string() +
            " does not exist or is not a directory.");
    }
    if( !bfs::exists(dest) )
    {
        bfs::create_directory(dest);
    }
    // Iterate through the source directory
    static const bfs::directory_iterator end_it = bfs::directory_iterator();
    for( bfs::directory_iterator file(src); file != end_it; ++file )
    {
        bfs::path current(file->path());
        if (!bfs::exists(current))
        {
            LOG(WARNING) << "the file disappeared while coping: " << current;
            continue;
        }
        if(bfs::is_directory(current))
        {
            // copy recursion
            copyDir(current, dest / current.filename());
        }
        else
        {
            if (current.filename().string().find("_removed.rollback") != std::string::npos)
                continue;

            DistributeTestSuit::testFail(Fail_At_CopyRemove_File);
            // Found file: Copy
            bfs::copy_file(current, dest / current.filename(), bfs::copy_option::overwrite_if_exists);
            //LOG(INFO) << "copying : " << current << " to " << dest;
        }
    }
    // remove the file that exists in dest but not in src.
    for ( bfs::directory_iterator file(dest); file != end_it; ++file )
    {
        bfs::path current(file->path());
        if (!bfs::exists(src / current.filename()))
        {
            bfs::remove_all(current);
            LOG(INFO) << "path removed since not in src : " << current;
        }
    }
}

// copy_dir will keep src path filename
// src = xxx/xxx/src_name
// dest = xxx/xxx/xxx
// copy xxx/xxx/src_name to dest/src
// with keep_full_path = false copy xxx/xxx/src_name to dest/src_name
// if dest = xxx/xxx/xxx/src_name
// then copy src to dest
static void copy_dir(const bfs::path& src, const bfs::path& dest, bool keep_full_path = true)
{
    bfs::path dest_path = dest;
    if (src.filename() == dest_path.filename())
    {
    }
    else if (!keep_full_path)
    {
        dest_path /= src.filename();
    }
    else
    {
        dest_path /= src;
    }
    bfs::create_directories(dest_path);
    copyDir(src, dest_path);
}

bool RecoveryChecker::getCollPath(const std::string& colname, CollectionPath& colpath)
{
    boost::unique_lock<boost::mutex> lock(mutex_);
    CollInfoMapT::const_iterator cit = all_col_info_.find(colname);
    if (cit == all_col_info_.end())
        return false;
    colpath = cit->second.first;
    return true;
}

void RecoveryChecker::addCollection(const std::string& colname, const CollectionPath& colpath, const std::string& configfile)
{
    boost::unique_lock<boost::mutex> lock(mutex_);
    LOG(INFO) << "collection added, updating in RecoveryChecker." << colname;
    all_col_info_[colname] = std::make_pair(colpath, configfile);
}

void RecoveryChecker::removeCollection(const std::string& colname)
{
    boost::unique_lock<boost::mutex> lock(mutex_);
    LOG(INFO) << "collection removed, updating in RecoveryChecker." << colname;
    all_col_info_.erase(colname);
}


bool RecoveryChecker::setRollbackFlag(uint32_t inc_id)
{
    if (bfs::exists(rollback_file_))
    {
        LOG(ERROR) << "rollback_file already exist, this mean last rollback is not finished. must exit.";
        forceExit("set rollback flag failed for existed flag.");
    }
    ofstream ofs(rollback_file_.c_str());
    if (ofs.good())
    {
        ofs.write((const char*)&inc_id, sizeof(inc_id));
        return true;
    }
    return false;
}

void RecoveryChecker::clearRollbackFlag()
{
    if (bfs::exists(rollback_file_))
        bfs::remove(rollback_file_);
}

bool RecoveryChecker::backup()
{
    boost::unique_lock<boost::mutex> lock(mutex_);
    if (!reqlog_mgr_)
    {
        LOG(ERROR) << "RecoveryChecker did not init!";
        return false;
    }
    // backup changeable data first, so that we can rollback if old data corrupt while process the request.
    // Ignore SCD files and any other files that will not change during processing.

    // find all collection and backup.
    CollInfoMapT::const_iterator cit = all_col_info_.begin();
    bfs::path dest_path = backup_basepath_ + "/" + boost::lexical_cast<std::string>(reqlog_mgr_->getLastSuccessReqId());
    bfs::path dest_coldata_backup = dest_path/bfs::path("backup_data");

    if (bfs::exists(dest_path))
        CopyGuard::safe_remove_all(dest_path.string());
    bfs::create_directories(dest_path);

    {
        CopyGuard dir_guard(dest_path.string());

        bfs::create_directories(dest_coldata_backup);

        // set invalide flag before do copy and
        // clear invalide flag after copy. So we can
        // know whether copy is finished correctly by checking flag .
        //
        while(cit != all_col_info_.end())
        {
            LOG(INFO) << "backing up the collection: " << cit->first;
            // flush collection to make sure all changes have been saved to disk.
            if (flush_col_)
                flush_col_(cit->first);
            if(!backupColl(cit->second.first, dest_path))
            {
                return false;
            }
            ++cit;
        }
        try
        {
            copy_dir(request_log_basepath_, dest_path, false);
        }
        catch(const std::exception& e)
        {
            // clear state.
            LOG(ERROR) << "backup request log failed. " << e.what() << std::endl;
            return false;
        }
    }

    cleanUnnessesaryBackup(backup_basepath_);
    return true;
}

bool RecoveryChecker::redoLog(ReqLogMgr* redolog, uint32_t start_id)
{
    bool ret = true;
    try
    {
        // read redo log and do request to current log.
        if (start_id > redolog->getLastSuccessReqId())
        {
            LOG(ERROR) << "backup id should not larger than last write id. " << start_id;
            LOG(ERROR) << "leaving old backup and do not redo log. restart and wait to sync to newest log";
            forceExit("redo start id is larger than last writed id.");
        }
        ReqLogHead rethead;
        size_t redo_offset = 0;
        if(!redolog->getHeadOffset(start_id, rethead, redo_offset))
        {
            assert(false);
            LOG(ERROR) << "get head failed. This should never happen.";
            forceExit("redo get head info failed.");
        }
        while(true)
        {
            std::string req_packed_data; 
            bool hasmore = redolog->getReqDataByHeadOffset(redo_offset, rethead, req_packed_data);
            if (!hasmore)
                break;
            LOG(INFO) << "redoing for request id : " << rethead.inc_id;
            CommonReqData req_commondata;
            ReqLogMgr::unpackReqLogData(req_packed_data, req_commondata);
            if(!DistributeDriver::get()->handleReqFromLog(req_commondata.reqtype, req_commondata.req_json_data, req_packed_data))
            {
                LOG(INFO) << "redoing from log failed: " << rethead.inc_id;
                forceExit("handleReqFromLog failed.");
            }
        }
    }
    catch(const std::exception& e)
    {
        ret = false;
    }
    bfs::remove_all(redo_log_basepath_);
    return ret;
}

bool RecoveryChecker::backupColl(const CollectionPath& colpath, const bfs::path& dest_path)
{
    // find all collection and backup.
    bfs::path dest_coldata_backup = dest_path/bfs::path("backup_data");

    if (!bfs::exists(dest_path))
    {
        bfs::create_directories(dest_path);
        bfs::create_directories(dest_coldata_backup);
    }

    bfs::path coldata_path(colpath.getCollectionDataPath());
    bfs::path querydata_path(colpath.getQueryDataPath());

    try
    {
        copy_dir(coldata_path, dest_coldata_backup);
        copy_dir(querydata_path, dest_coldata_backup);
    }
    catch(const std::exception& e)
    {
        LOG(ERROR) << "backup collection data failed. " << e.what() << std::endl;
        return false;
    }
    return true;
}

bool RecoveryChecker::hasAnyBackup()
{
    std::string last_backup_path;
    uint32_t last_backup_id = 0;
    bool has_backup = true;
    if (!getLastBackup(backup_basepath_, last_backup_path, last_backup_id))
    {
        last_backup_id = 0;
        has_backup = false;
    }
    return has_backup;
}

bool RecoveryChecker::checkAndRestoreBackupFile(const CollectionPath& colpath)
{
    if (!bfs::exists(rollback_file_))
    {
        return true;
    }
    ifstream ifs(rollback_file_.c_str());
    if (!ifs.good())
    {
        return false;
    }

    std::string last_backup_path;
    uint32_t last_backup_id = 0;
    bool has_backup = true;
    if (!getLastBackup(backup_basepath_, last_backup_path, last_backup_id))
    {
        last_backup_id = 0;
        has_backup = false;
    }
    if (has_backup)
    {
        // copy backup data to replace current and reopen all file.
        try
        {
            bfs::path dest_coldata_backup(last_backup_path + "/backup_data");
            LOG(INFO) << "restoring the backup for the collection.";

            bfs::path coldata_path(colpath.getCollectionDataPath());
            bfs::path querydata_path(colpath.getQueryDataPath());

            copy_dir(dest_coldata_backup/coldata_path, coldata_path);
            copy_dir(dest_coldata_backup/querydata_path, querydata_path);
        }
        catch(const std::exception& e)
        {
            LOG(ERROR) << "copy backup to current working path failed." << e.what();
            return false;
        }
    }
    return true;
}

// note : if there are more than one collection we need backup
// all collection data. only in this way we can rollback and redo log to 
// rollback the data before last failed request.
bool RecoveryChecker::rollbackLastFail(bool need_restore_backupfile)
{
    // rollback will only happen in recovery or abort request, both of them 
    // are protected by the lock of node state in NodeManagerBase. 
    // not all inc_id has a backup, so we just find the newest backup.
    if (!bfs::exists(rollback_file_))
    {
        LOG(INFO) << "rollback flag not exist, no need to rollback.";
        return true;
    }
    ifstream ifs(rollback_file_.c_str());
    if (!ifs.good())
    {
        LOG(ERROR) << "read rollback flag file error.";
        return false;
    }
    if (bfs::exists(redo_log_basepath_))
    {
        LOG(INFO) << "clean redo log path !!";
        bfs::remove_all(redo_log_basepath_);
    }
    uint32_t rollback_id;
    ifs.read((char*)&rollback_id, sizeof(rollback_id));
    LOG(INFO) << "last failed request inc_id is :" << rollback_id;

    std::string last_backup_path;
    uint32_t last_backup_id = 0;
    uint32_t  last_write_id = reqlog_mgr_->getLastSuccessReqId();
    // mv current log to redo log, copy backup log to current, 
    bfs::rename(request_log_basepath_, redo_log_basepath_);
    ReqLogMgr redo_req_log_mgr;
    redo_req_log_mgr.init(redo_log_basepath_);
    bool has_backup = true;
    if (!getLastBackup(backup_basepath_, last_backup_path, last_backup_id))
    {
        LOG(ERROR) << "no backup available while rollback.";
        LOG(ERROR) << "need restart to redo all log or get full data from primary.";
        // remove all data and redo all log.
        last_backup_id = 0;
        has_backup = false;
    }

    if (has_backup)
    {
        // copy backup log to current .
        copy_dir(last_backup_path + bfs::path(request_log_basepath_).filename().c_str(),
            request_log_basepath_, false);
    }
    if (has_backup && need_restore_backupfile)
    {
        CollInfoMapT tmp_all_col_info;
        {
            boost::unique_lock<boost::mutex> lock(mutex_);
            // stop collection will change the metamap so we need a copy. 
            tmp_all_col_info = all_col_info_;
        }
        CollInfoMapT::const_iterator cit = tmp_all_col_info.begin();
        while(cit != tmp_all_col_info.end())
        {
            stop_col_(cit->first, true);
            ++cit;
        }

        // copy backup data to replace current and reopen all file.
        try
        {
            CollInfoMapT::const_iterator inner_cit = tmp_all_col_info.begin();
            bfs::path dest_coldata_backup(last_backup_path + "/backup_data");
            while(inner_cit != tmp_all_col_info.end())
            {
                LOG(INFO) << "restoring the backup for the collection: " << inner_cit->first;
                const CollectionPath &colpath = inner_cit->second.first;

                bfs::path coldata_path(colpath.getCollectionDataPath());
                bfs::path querydata_path(colpath.getQueryDataPath());

                copy_dir(dest_coldata_backup/coldata_path, coldata_path);
                copy_dir(dest_coldata_backup/querydata_path, querydata_path);
                ++inner_cit;
            }
        }
        catch(const std::exception& e)
        {
            LOG(ERROR) << "copy backup to current working path failed." << e.what();
            bfs::rename(redo_log_basepath_, request_log_basepath_);
            return false;
        }

        cit = tmp_all_col_info.begin();
        while(cit != tmp_all_col_info.end())
        {
            if(!start_col_(cit->first, cit->second.second))
                return false;
            ++cit;
        }
    }

    if (rollback_id != 0)
    {
        assert(last_backup_id < rollback_id);
    }
    LOG(INFO) << "rollback from " << rollback_id << " to backup: " << last_backup_id;
    reqlog_mgr_->init(request_log_basepath_);

    LOG(INFO) << "rollback to last backup finished, begin redo from backup to sync to last fail request.";
    clearRollbackFlag();

    if (last_backup_id == last_write_id)
    {
        LOG(INFO) << "last backup is up to newest log. no redo need.";
    }
    else if (rollback_id == 0 || last_write_id == 0)
    {
        LOG(INFO) << "rollback id is 0, this rollback caused by corrupt request log, no redo need.";
    }
    else
    {
        LOG(INFO) << "last backup is out of date. begin redo request.";
        // read redo log and do request to current log.
        last_backup_id++;
        return redoLog(&redo_req_log_mgr, last_backup_id);
    }
    bfs::remove_all(redo_log_basepath_);
    return true;
}

void RecoveryChecker::init(const std::string& workdir)
{
    backup_basepath_ = workdir + "/req-backup";
    request_log_basepath_ = workdir + "/req-log";
    redo_log_basepath_ = workdir + "/redo-log";
    rollback_file_ = workdir + "/rollback_flag";

    reqlog_mgr_.reset(new ReqLogMgr());
    try
    {
        reqlog_mgr_->init(request_log_basepath_);
    }
    catch(const std::exception& e)
    {
        LOG(ERROR) << "request log init error, need clear corrupt data and rollback to last backup.";
        setRollbackFlag(0);
        bfs::remove_all(request_log_basepath_);
        reqlog_mgr_->init(request_log_basepath_);
    }

    DistributeTestSuit::updateMemoryState("Last_Success_Request", reqlog_mgr_->getLastSuccessReqId());

    NodeManagerBase::get()->setRecoveryCallback(
        boost::bind(&RecoveryChecker::onRecoverCallback, this, _1),
        boost::bind(&RecoveryChecker::onRecoverWaitPrimaryCallback, this),
        boost::bind(&RecoveryChecker::onRecoverWaitReplicasCallback, this));

}

void RecoveryChecker::onRecoverCallback(bool startup)
{
    LOG(INFO) << "recovery callback, begin recovery before enter cluster.";

    // If myself need rollback, we must make sure primary node is available.
    //
    // startup is true means recovery from first start up. otherwise means recovery from
    // re-enter cluster.
    if (startup && (bfs::exists(rollback_file_) || !isLastNormalExit()) )
    {
        LOG(INFO) << "recovery from rollback or from last forceExit !!";
        if (!NodeManagerBase::get()->isOtherPrimaryAvailable())
            forceExit("recovery failed. No primary Node!!");
    }

    if(!rollbackLastFail(!startup))
    {
        LOG(ERROR) << "corrupt data and rollback failed. Unrecoverable!!";
        forceExit("corrupt data and rollback failed. Unrecoverable!!");
    }
    setForceExitFlag();
    syncSCDFiles();
    syncToNewestReqLog();

    CollInfoMapT tmp_all_col_info;
    {
        boost::unique_lock<boost::mutex> lock(mutex_);
        tmp_all_col_info = all_col_info_;
    }
    CollInfoMapT::const_iterator cit = tmp_all_col_info.begin();
    while(cit != tmp_all_col_info.end())
    {
        if (flush_col_)
            flush_col_(cit->first);
        ++cit;
    }

    // if failed, must exit.
    LOG(INFO) << "recovery finished, wait primary agree before enter cluster.";
}

bool RecoveryChecker::checkDataConsistent()
{
    // check data consistent with primary.
    std::string errinfo;
    CollInfoMapT tmp_all_col_info;
    {
        boost::unique_lock<boost::mutex> lock(mutex_);
        tmp_all_col_info = all_col_info_;
    }

    CollInfoMapT::const_iterator cit = tmp_all_col_info.begin();
    while(cit != tmp_all_col_info.end())
    {
        DistributeFileSyncMgr::get()->checkReplicasStatus(cit->first, errinfo);
        if (!errinfo.empty())
        {
            LOG(ERROR) << "data is not consistent after recovery, collection : " << cit->first <<
                ", error : " << errinfo;
            return false;
        }
        ++cit;
    }
    return true;
}

void RecoveryChecker::onRecoverWaitPrimaryCallback()
{
    LOG(INFO) << "primary agreed , sync new request druing waiting recovery.";
    // check new request during wait recovery.
    syncSCDFiles();
    syncToNewestReqLog();
    LOG(INFO) << "primary agreed and my recovery finished, begin enter cluster";
    // check data consistent with primary.
    std::string errinfo;
    CollInfoMapT tmp_all_col_info;
    {
        boost::unique_lock<boost::mutex> lock(mutex_);
        tmp_all_col_info = all_col_info_;
    }
    bool sync_file = bfs::exists("./distribute_sync_file.flag");

    CollInfoMapT::const_iterator cit = tmp_all_col_info.begin();
    while(cit != tmp_all_col_info.end())
    {
        if (flush_col_)
            flush_col_(cit->first);
        DistributeFileSyncMgr::get()->checkReplicasStatus(cit->first, errinfo);
        if (!errinfo.empty())
        {
            setRollbackFlag(0);
            LOG(ERROR) << "data is not consistent after recovery, collection : " << cit->first <<
                ", error : " << errinfo;
            if (sync_file)
            {
                if(!DistributeFileSyncMgr::get()->syncCollectionData(cit->first))
                    forceExit("recovery failed for sync collection file.");
            }
            else
                forceExit("recovery failed for not consistent."); 
            clearRollbackFlag();
        }
        ++cit;
    }
    if (sync_file)
        bfs::remove("./distribute_sync_file.flag");

}

void RecoveryChecker::onRecoverWaitReplicasCallback()
{
    LOG(INFO) << "all recovery replicas has entered the cluster after recovery finished.";
    LOG(INFO) << "wait recovery finished , primary ready to server";
}

void RecoveryChecker::syncSCDFiles()
{
    CollInfoMapT tmp_all_col_info;
    {
        boost::unique_lock<boost::mutex> lock(mutex_);
        tmp_all_col_info = all_col_info_;
    }
    CollInfoMapT::const_iterator cit = tmp_all_col_info.begin();
    while(cit != tmp_all_col_info.end())
    {
        LOG(INFO) << "begin sync scd files for collection : " << cit->first;
        DistributeFileSyncMgr::get()->syncNewestSCDFileList(cit->first);
        ++cit;
    }
}

void RecoveryChecker::syncToNewestReqLog()
{
    LOG(INFO) << "begin sync to newest log.";
    std::vector<std::string> newlogdata_list;
    while(true)
    {
        uint32_t reqid = reqlog_mgr_->getLastSuccessReqId();
        DistributeFileSyncMgr::get()->getNewestReqLog(reqid + 1, newlogdata_list);
        if (newlogdata_list.empty())
            break;
        for (size_t i = 0; i < newlogdata_list.size(); ++i)
        {
            // do new redo RequestLog.
            CommonReqData req_commondata;
            ReqLogMgr::unpackReqLogData(newlogdata_list[i], req_commondata);
            LOG(INFO) << "redoing for request id : " << req_commondata.inc_id;
            if (req_commondata.inc_id <= reqid)
            {
                LOG(ERROR) << "get new log data is out of order.";
                forceExit("syncToNewestReqLog failed.");
            }
            reqid = req_commondata.inc_id;
            if(!DistributeDriver::get()->handleReqFromLog(req_commondata.reqtype, req_commondata.req_json_data, newlogdata_list[i]))
            {
                LOG(INFO) << "redoing from log failed: " << req_commondata.inc_id;
                forceExit("handleReqFromLog failed.");
            }
        }
    }
    LOG(INFO) << "no more new log to redo. syncd to : " << reqlog_mgr_->getLastSuccessReqId();
}

bool RecoveryChecker::isLastNormalExit()
{
    return !bfs::exists("./distribute_force_exit.flag");
}

void RecoveryChecker::clearForceExitFlag()
{
    LOG(INFO) << "forceExit flag cleared!";
    if (bfs::exists("./distribute_force_exit.flag"))
    {
        bfs::remove("./distribute_force_exit.flag");
    }
}

void RecoveryChecker::setForceExitFlag()
{
    LOG(INFO) << "setting forceExit flag!";
    std::ofstream ofs("./distribute_force_exit.flag", ios::app|ios::binary|ios::ate|ios::out);
    ofs << std::flush;
    ofs.close();
}

void RecoveryChecker::forceExit(const std::string& err)
{
    setForceExitFlag();
    if (err.empty())
        throw std::runtime_error("force exit in RecoveryChecker.");
    throw std::runtime_error(err.c_str());
}

}

#include "RecoveryChecker.h"
#include "DistributeFileSyncMgr.h"
#include "DistributeDriver.h"
#include "RequestLog.h"
#include "NodeManagerBase.h"
#include "DistributeTest.hpp"

#include <glog/logging.h>
#include <boost/filesystem.hpp>
#include <common/Keys.h>
#include <util/driver/Reader.h>
#include <util/driver/Writer.h>
#include <util/driver/writers/JsonWriter.h>
#include <util/driver/readers/JsonReader.h>

#define MAX_BACKUP_NUM 3
namespace bfs = boost::filesystem;

using namespace izenelib::driver;

namespace sf1r
{

class CopyGuard
{
public:
    CopyGuard(const std::string& guard_dir)
        : ok_(false), guard_file_(guard_dir + "/" + getGuardFileName())
    {
        // touch a flag
        std::ofstream ofs(guard_file_.c_str());
        ofs << std::flush;
        ofs.close();
    }
    ~CopyGuard()
    {
        // clear a flag
        if (ok_ && bfs::exists(guard_file_))
        {
            bfs::remove(guard_file_);
        }
    }
    void setOK()
    {
        ok_ = true;
    }
    static bool isDirCopyOK(const std::string& dir)
    {
        return !bfs::exists(dir + "/" + getGuardFileName());
    }
    static void safe_remove_all(const std::string& dir)
    {
        CopyGuard dir_guard(dir);
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
        dir_guard.setOK();
    }

    static std::string getGuardFileName()
    {
        static std::string guard_flag("copy_guard_file.flag");
        return guard_flag;
    }

private:
    bool ok_;
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
        if (keeped >= MAX_BACKUP_NUM)
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

static bool getLastBackup(const bfs::path& backup_basepath, uint32_t less_than, std::string& backup_path, uint32_t& backup_inc_id)
{
    std::vector<uint32_t> backup_req_incids;
    getBackupList(backup_basepath, backup_req_incids);
    if (backup_req_incids.empty())
        return false;
    LOG(INFO) << "getting last backup id which is less than : " << less_than;
    for (size_t i = 0; i < backup_req_incids.size(); ++i)
    {
        backup_inc_id = backup_req_incids[i];
        backup_path = (backup_basepath/bfs::path(boost::lexical_cast<std::string>(backup_inc_id))).string() + "/";
        if (CopyGuard::isDirCopyOK(backup_path))
        {
            if (less_than == 0 || backup_inc_id < less_than)
                return true;
        }
        else 
        {
            LOG(WARNING) << "a corrupted directory removed " << backup_path;
            CopyGuard::safe_remove_all(backup_path);
        }
    }
    return false;
}

static void copy_file_keep_modification(const bfs::path& from, const bfs::path& to)
{
    bfs::copy_file(from, to, bfs::copy_option::overwrite_if_exists);
    try
    {
        bfs::last_write_time(to, bfs::last_write_time(from));
    }catch(const std::exception& e){}
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
            copy_file_keep_modification(current, dest / current.filename());
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

void RecoveryChecker::getCollList(std::vector<std::string>& coll_list)
{
    boost::unique_lock<boost::mutex> lock(mutex_);
    CollInfoMapT::const_iterator cit = all_col_info_.begin();
    while (cit != all_col_info_.end())
    {
        coll_list.push_back(cit->first);
        ++cit;
    }
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

bool RecoveryChecker::backup(bool force_remove)
{
    CollInfoMapT tmp_all_col_info;
    {
        boost::unique_lock<boost::mutex> lock(mutex_);
        if (!reqlog_mgr_)
        {
            LOG(ERROR) << "RecoveryChecker did not init!";
            return false;
        }
        need_backup_ = false;
        // find all collection and backup.
        tmp_all_col_info = all_col_info_;
    }

    // backup changeable data first, so that we can rollback if old data corrupt while process the request.
    // Ignore SCD files and any other files that will not change during processing.
    CollInfoMapT::const_iterator cit = tmp_all_col_info.begin();
    bfs::path dest_path = backup_basepath_ + "/" + boost::lexical_cast<std::string>(reqlog_mgr_->getLastSuccessReqId());
    bfs::path dest_coldata_backup = dest_path/bfs::path("backup_data");

    if (bfs::exists(dest_path))
    {
        if (CopyGuard::isDirCopyOK(dest_path.string()) && !force_remove)
        {
            LOG(INFO) << "backup already exists : " << dest_path;
            return true;
        }
        CopyGuard::safe_remove_all(dest_path.string());
    }
    bfs::create_directories(dest_path);

    {
        CopyGuard dir_guard(dest_path.string());

        bfs::create_directories(dest_coldata_backup);

        // set invalide flag before do copy and
        // clear invalide flag after copy. So we can
        // know whether copy is finished correctly by checking flag .
        //
        while(cit != tmp_all_col_info.end())
        {
            LOG(INFO) << "backing up the collection: " << cit->first;
            // flush collection to make sure all changes have been saved to disk.
            if (flush_col_)
                flush_col_(cit->first);

            LOG(INFO) << "flush the collection finished.";
            if(!backupColl(cit->second.first, dest_path))
            {
                return false;
            }
            ++cit;
        }
        try
        {
            copy_dir(request_log_basepath_, dest_path, false);
            copy_file_keep_modification(last_conf_file_, dest_path/bfs::path(last_conf_file_).filename());
        }
        catch(const std::exception& e)
        {
            // clear state.
            LOG(ERROR) << "backup request log failed. " << e.what() << std::endl;
            return false;
        }
        dir_guard.setOK();
    }

    LOG(INFO) << "back up finished.";
    cleanUnnessesaryBackup(backup_basepath_);
    return true;
}

void RecoveryChecker::replayLog(bool is_primary, const std::string& from_col,
    const std::string& to_col, std::vector<uint32_t>& replayed_id_list)
{
    // replay log, ignore any fail and do not write any log during replay.
    // read redo log and do request to current log.
    uint32_t start_id = 0;
    uint32_t end_id = reqlog_mgr_->getLastSuccessReqId();
    if (start_id > end_id)
    {
        LOG(ERROR) << "no log replay.";
        return;
    }
    ReqLogHead rethead;
    size_t redo_offset = 0;
    if(!reqlog_mgr_->getHeadOffset(start_id, rethead, redo_offset))
    {
        LOG(ERROR) << "get start head failed." << start_id;
        return;
    }

    LOG(INFO) << "replaying log is_primary: " << is_primary << ", from collection: " << from_col <<
        ", to collection : " << to_col;
    if (is_primary)
        replayed_id_list.reserve(end_id - start_id);
    else
    {
        if (replayed_id_list.empty())
        {
            LOG(INFO) << "nothing log need replay.";
            return;
        }
    }


    uint32_t replayed_num = 0;

    while(true)
    {
        std::string req_packed_data; 
        CommonReqData req_commondata;
        try
        {
            bool hasmore = reqlog_mgr_->getReqDataByHeadOffset(redo_offset, rethead, req_packed_data);
            if (!hasmore || rethead.inc_id > end_id)
            {
                if (rethead.inc_id < end_id)
                    end_id = rethead.inc_id;
                LOG(INFO) << "end at inc_id : " << end_id;
                break;
            }
            if (!is_primary)
            {
                // only replay the id that primary has replayed.
                if (rethead.inc_id != replayed_id_list[replayed_num])
                {
                    LOG(INFO) << "ignore replay : " << rethead.inc_id;
                    continue;
                }
            }

            if(!ReqLogMgr::unpackReqLogData(req_packed_data, req_commondata))
            {
                LOG(INFO) << "unpack replaying log data failed : " << req_packed_data;
                if (!is_primary)
                    break;
                continue;
            }

            if (req_commondata.reqtype == Req_UpdateConfig ||
                req_commondata.reqtype == Req_CronJob ||
                req_commondata.reqtype == Req_Index ||
                req_commondata.reqtype == Req_Callback)
            {
                LOG(INFO) << "ignore replaying log type : " << req_commondata.reqtype;
                if (!is_primary)
                    break;
                continue;
            }

            // change collection .
            static izenelib::driver::JsonReader reader;
            Value requestValue;
            if(!reader.read(req_commondata.req_json_data, requestValue))
            {
                LOG(ERROR) << "read request json data error.";
                if (!is_primary)
                    break;
                continue;
            }
            if (requestValue.type() != Value::kObjectType)
            {
                LOG(ERROR) << "request value type error.";
                if (!is_primary)
                    break;
                continue;
            }
            Request request;
            request.assignTmp(requestValue);
            if (!ReqLogMgr::isReplayWriteReq(request.controller(), request.action()))
            {
                if (!is_primary)
                    break;
                continue;
            }
            std::string collection = asString(request[Keys::collection]);
            if (collection != from_col)
            {
                //LOG(INFO) << "not match collection : " << collection;
                if (!is_primary)
                    break;
                continue;
            }
            request[Keys::collection] = to_col;

            std::string replay_json;
            izenelib::driver::JsonWriter writer;
            writer.write(request.get(), replay_json);
            LOG(INFO) << "replaying for request id : " << rethead.inc_id;
            CommonReqData new_common = req_commondata;
            new_common.req_json_data = replay_json;
            ReqLogMgr::replaceCommonReqData(req_commondata, new_common, req_packed_data);

            if(!DistributeDriver::get()->handleReqFromLog(req_commondata.reqtype, replay_json, req_packed_data))
            {
                LOG(INFO) << "ignore replaying from log failed: " << rethead.inc_id << ", json : " << req_commondata.req_json_data;
                if (!is_primary)
                    break;
                continue;
            }
            replayed_num++;
            if (is_primary)
                replayed_id_list.push_back(rethead.inc_id);
        }
        catch(const std::exception& e)
        {
            LOG(INFO) << "replay log exception: " << e.what() << ", req data: " << req_commondata.req_json_data;
            if (!is_primary)
                break;
        }
    }
    if (!is_primary && replayed_num != replayed_id_list.size())
    {
        LOG(ERROR) << "replay log error on secondary.";
        forceExit("Replay Log Failed!");
    }
    LOG(INFO) << "replay log finished, total replayed: " << replayed_num;
}

bool RecoveryChecker::redoLog(ReqLogMgr* redolog, uint32_t start_id, uint32_t end_id)
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
            if (!hasmore || rethead.inc_id >= end_id)
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
        LOG(INFO) << "redo log exception: " << e.what();
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

bool RecoveryChecker::hasAnyBackup(uint32_t& last_backup_id)
{
    std::string last_backup_path;
    last_backup_id = 0;
    bool has_backup = true;
    if (!getLastBackup(backup_basepath_, 0, last_backup_path, last_backup_id))
    {
        last_backup_id = 0;
        has_backup = false;
    }
    return has_backup;
}

bool RecoveryChecker::isNeedRollback(bool starting_up)
{
    if (!NodeManagerBase::get()->isDistributed())
        return false;
    if (bfs::exists(rollback_file_))
        return true;
    if (starting_up && !isLastNormalExit())
    {
        return true;
    }
    return false;
}

bool RecoveryChecker::checkAndRestoreBackupFile(const CollectionPath& colpath)
{
    LOG(INFO) << "check and restoring backup at startup.";

    if(!isNeedRollback(true))
    {
        return true;
    }

    uint32_t rollback_id = 0;
    ifstream ifs(rollback_file_.c_str());
    if (ifs.good())
    {
        ifs.read((char*)&rollback_id, sizeof(rollback_id));
    }

    std::string last_backup_path;
    uint32_t last_backup_id = 0;
    bool has_backup = true;
    if (!getLastBackup(backup_basepath_, rollback_id, last_backup_path, last_backup_id))
    {
        LOG(ERROR) << "no backup available while check startup.";
        LOG(ERROR) << "need restart to redo all log or get full data from primary.";
        last_backup_id = 0;
        has_backup = false;
        return false;
    }
    if (has_backup)
    {
        // copy backup data to replace current and reopen all file.
        try
        {
            bfs::path dest_coldata_backup(last_backup_path + "/backup_data");
            LOG(INFO) << "restoring the backup for the collection: " <<
                colpath.getCollectionDataPath() << ", backup id: " << last_backup_id;

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
bool RecoveryChecker::rollbackLastFail(bool starting_up)
{
    // rollback will only happen in recovery or abort request, both of them 
    // are protected by the lock of node state in NodeManagerBase. 
    // not all inc_id has a backup, so we just find the newest backup.
    if (!isNeedRollback(starting_up))
    {
        LOG(INFO) << "no need to rollback.";
        return true;
    }

    uint32_t rollback_id = 0;
    ifstream ifs(rollback_file_.c_str());
    if (ifs.good())
    {
        ifs.read((char*)&rollback_id, sizeof(rollback_id));
    }

    if (bfs::exists(redo_log_basepath_))
    {
        LOG(INFO) << "clean redo log path !!";
        bfs::remove_all(redo_log_basepath_);
    }
    LOG(INFO) << "last failed request inc_id is :" << rollback_id;

    std::string last_backup_path;
    uint32_t last_backup_id = 0;
    uint32_t  last_write_id = reqlog_mgr_->getLastSuccessReqId();
    // mv current log to redo log, copy backup log to current, 
    bfs::rename(request_log_basepath_, redo_log_basepath_);
    ReqLogMgr redo_req_log_mgr;
    redo_req_log_mgr.init(redo_log_basepath_);
    bool has_backup = true;
    if (!getLastBackup(backup_basepath_, rollback_id, last_backup_path, last_backup_id))
    {
        LOG(ERROR) << "no backup available while rollback.";
        LOG(ERROR) << "need restart to redo all log or get full data from primary.";
        // remove all data and redo all log.
        last_backup_id = 0;
        has_backup = false;
    }

    if (rollback_id != 0)
    {
        if (last_backup_id >= rollback_id)
        {
            LOG(ERROR) << "last backup write inc is larger than current failed inc_id: " <<
                last_backup_id << " vs " << rollback_id;
            bfs::rename(redo_log_basepath_, request_log_basepath_);
            return false;
        }
    }

    if (has_backup)
    {
        // copy backup log to current .
        copy_dir(last_backup_path + bfs::path(request_log_basepath_).filename().c_str(),
            request_log_basepath_, false);
    }
    if (has_backup && !starting_up)
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

        LOG(INFO) << "rollback begin update config file and restart the collection." << last_backup_path;
        // check if config file changed. if so, we need restart to finish rollback.
        std::string restore_conf_file = last_backup_path + bfs::path(last_conf_file_).filename().string();
        copy_file_keep_modification(restore_conf_file, last_conf_file_);

        std::map<std::string, std::string> new_running_col = handleConfigUpdate();

        std::map<std::string, std::string>::const_iterator new_cit = new_running_col.begin();
        while(new_cit != new_running_col.end())
        {
            if(!start_col_(new_cit->first, new_cit->second, true))
            {
                bfs::rename(redo_log_basepath_, request_log_basepath_);
                return false;
            }
            ++new_cit;
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
        return redoLog(&redo_req_log_mgr, last_backup_id, rollback_id);
    }
    bfs::remove_all(redo_log_basepath_);
    return true;
}

void RecoveryChecker::init(const std::string& conf_dir, const std::string& workdir)
{
    backup_basepath_ = workdir + "/req-backup";
    request_log_basepath_ = workdir + "/req-log";
    redo_log_basepath_ = workdir + "/redo-log";
    rollback_file_ = workdir + "/rollback_flag";
    last_conf_file_ = workdir + "/distribute_last_conf";
    configDir_ = conf_dir;
    need_backup_ = false;
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

    if (bfs::exists(rollback_file_))
    {
        ifstream ifs(rollback_file_.c_str());
        if (ifs.good())
        {
            uint32_t rollback_id = 0;
            ifs.read((char*)&rollback_id, sizeof(rollback_id));

            LOG(INFO) << "rollback at startup, restore the last config file for the rollback." << rollback_id;
            std::string last_backup_path;
            uint32_t last_backup_id = 0;
            if (getLastBackup(backup_basepath_, rollback_id, last_backup_path, last_backup_id))
            {
                copy_file_keep_modification(last_backup_path + bfs::path(last_conf_file_).filename().string(), last_conf_file_);
                LOG(INFO) << "restore config file from : " << last_backup_path;
            }
            else
            {
                LOG(ERROR) << "no backup available while check startup.";
            }
        }
    }

    handleConfigUpdate();

    DistributeTestSuit::updateMemoryState("Last_Success_Request", reqlog_mgr_->getLastSuccessReqId());

    NodeManagerBase::get()->setRecoveryCallback(
        boost::bind(&RecoveryChecker::onRecoverCallback, this, _1),
        boost::bind(&RecoveryChecker::onRecoverWaitPrimaryCallback, this),
        boost::bind(&RecoveryChecker::onRecoverWaitReplicasCallback, this),
        boost::bind(&RecoveryChecker::onRecoverCheckLog, this, _1));

}

void RecoveryChecker::flushAllData()
{   
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
}

void RecoveryChecker::onRecoverCallback(bool startup)
{
    LOG(INFO) << "recovery callback, begin recovery before enter cluster.";

    bool need_rollback = bfs::exists(rollback_file_);
    if(!rollbackLastFail(startup))
    {
        LOG(ERROR) << "corrupt data and rollback failed. Unrecoverable!!";
        forceExit("corrupt data and rollback failed. Unrecoverable!!");
    }

    // If myself need rollback, we must make sure primary node is available.
    //
    // startup is true means recovery from first start up. otherwise means recovery from
    // re-enter cluster.
    if (startup && (need_rollback || !isLastNormalExit()) )
    {
        flushAllData();
        LOG(INFO) << "recovery from rollback or from last forceExit !!";
        if (reqlog_mgr_->getLastSuccessReqId() > 0 && !NodeManagerBase::get()->isOtherPrimaryAvailable())
            forceExit("recovery failed. No primary Node!!");
    }

    if (NodeManagerBase::get()->isOtherPrimaryAvailable() && !checkIfLogForward(false))
    {
        LOG(INFO) << "check log failed while recover, need rollback";
        if(!rollbackLastFail(false))
        {
            forceExit("rollback failed for forword log.");
        }
    }

    setForceExitFlag();
    syncSCDFiles();
    syncToNewestReqLog();

    // if failed, must exit.
    LOG(INFO) << "recovery finished, wait primary agree before enter cluster.";
}

bool RecoveryChecker::removeConfigFromAPI(const std::string& coll)
{
    if (!NodeManagerBase::get()->isDistributed())
    {
        return true;
    }

    std::map<std::string, std::string> config_file_list;
    LOG(INFO) << "remove config file for the collection." << coll;
    if(!handleConfigUpdateForColl(coll, true, config_file_list))
        return false;

    UpdateConfigReqLog new_config_log;
    new_config_log.inc_id = 0;
    new_config_log.config_file_list = config_file_list;
    std::string new_conf_data;
    ReqLogMgr::packReqLogData(new_config_log, new_conf_data);

    std::ofstream ofs;
    ofs.open(last_conf_file_.c_str(), ios::binary);
    ofs << new_conf_data << std::flush;
    ofs.close();

    return true;
}

bool RecoveryChecker::updateConfigFromAPI(const std::string& coll, bool is_primary,
    const std::string& config_file, std::map<std::string, std::string>& config_file_list)
{
    if (!NodeManagerBase::get()->isDistributed())
    {
        return true;
    }
    if (!is_primary)
    {
        LOG(INFO) << "write updated config data to last conf file on the secondary.";
    }
    else
    {
        LOG(INFO) << "begin update config file on primary for the collection." << coll;
        if(!handleConfigUpdateForColl(coll, false, config_file_list))
            return false;
    }

    UpdateConfigReqLog new_config_log;
    new_config_log.inc_id = 0;
    new_config_log.config_file_list = config_file_list;
    std::string new_conf_data;
    ReqLogMgr::packReqLogData(new_config_log, new_conf_data);

    std::ofstream ofs;
    ofs.open(last_conf_file_.c_str(), ios::binary);
    ofs << new_conf_data << std::flush;
    ofs.close();

    handleConfigUpdate();

    return true;
}

bool RecoveryChecker::handleConfigUpdateForColl(const std::string& coll, bool delete_conf,
    std::map<std::string, std::string>& config_file_list)
{
    // read all old config file.
    std::ifstream last_conf;
    last_conf.open(last_conf_file_.c_str(), ios::binary);
    if (!last_conf.good())
    {
        LOG(ERROR) << "last config not found.";
        return false;
    }
    last_conf.seekg(0, ios::end);
    size_t len = last_conf.tellg();
    last_conf.seekg(0, ios::beg);
    std::string old_conf_data;
    old_conf_data.resize(len);
    last_conf.read((char*)&old_conf_data[0], len);
    UpdateConfigReqLog last_conf_log;
    if(!ReqLogMgr::unpackReqLogData(old_conf_data, last_conf_log))
    {
        LOG(ERROR) << "unpack last config data error";
        return false;
    }
    last_conf.close();
    config_file_list = last_conf_log.config_file_list;

    // update config for the specific collection.
    bfs::path current(configDir_);
    current /= coll + ".xml";

    if (delete_conf)
    {
        config_file_list.erase(current.filename().string());
        if (bfs::is_regular_file(current))
            bfs::remove(current);
        LOG(INFO) << "config file removed : " << current;
        return true;
    }

    if(!bfs::is_regular_file(current))
    {
        LOG(INFO) << "updating config file not found : " << current;
        return false;
    }

    std::string filename = current.filename().string();
    std::string& current_conf_file = config_file_list[filename];

    LOG(INFO) << "updating config for coll: " << coll;

    std::ifstream cur_file_ifs;
    cur_file_ifs.open(current.string().c_str(), ios::binary);
    cur_file_ifs.seekg(0, ios::end);
    size_t cur_len = cur_file_ifs.tellg();
    cur_file_ifs.seekg(0, ios::beg);
    current_conf_file.resize(cur_len);
    cur_file_ifs.read((char*)&current_conf_file[0], cur_len);
    cur_file_ifs.close();
    return true;
}

std::map<std::string, std::string> RecoveryChecker::handleConfigUpdate()
{
    // check if last config exist, if no we started as first time. save current
    // config to last config. if last config is different from current config we
    // need to update this to log file to notify other replicas if current starting
    // node is primary. If not primary, we should not modify the config. And the secondary
    // need restart twice to update the new config from primary.
    // the first start on secondary will update the new config from primary.
    // the second start will use the new config while starting.
    bool need_update_config = false;
    if (!bfs::exists(last_conf_file_))
    {
        LOG(INFO) << " No last config, starting as first time.";
        need_backup_ = true;
        need_update_config = true;
    }
    else
    {
        LOG(INFO) << " Starting using the last unchanged config.";
    }

    std::map<std::string, std::string> running_col_info;

    UpdateConfigReqLog cur_conf_log;
    cur_conf_log.inc_id = 0;
    std::map<std::string, std::string>& current_conf_files = cur_conf_log.config_file_list;
    bfs::directory_iterator iter(configDir_), end_iter;
    for(; iter!= end_iter; ++iter)
    {
        bfs::path current(iter->path());
        if(bfs::is_regular_file(current))
        {
            if(current.filename().string().rfind(".xml") == (current.filename().string().length() - std::string(".xml").length()))
            {
                std::string filename = current.filename().string();
                LOG(INFO) << "find a config file for : " << current;
                if (filename == "sf1config.xml")
                {
                    continue;
                }
                std::ifstream cur_file_ifs;
                cur_file_ifs.open(current.string().c_str(), ios::binary);
                cur_file_ifs.seekg(0, ios::end);
                size_t cur_len = cur_file_ifs.tellg();
                cur_file_ifs.seekg(0, ios::beg);
                current_conf_files[filename].resize(cur_len);
                cur_file_ifs.read((char*)&current_conf_files[filename][0], cur_len);
                cur_file_ifs.close();
                running_col_info[filename.substr(0, filename.length() - 4)] = current.string();
            }
        }
    }

    if (!need_update_config)
    {
        std::ifstream last_conf;
        last_conf.open(last_conf_file_.c_str(), ios::binary);
        if (!last_conf.good())
        {
            forceExit("reading last config file error");
        }
        last_conf.seekg(0, ios::end);
        size_t len = last_conf.tellg();
        last_conf.seekg(0, ios::beg);
        std::string old_conf_data;
        old_conf_data.resize(len);
        last_conf.read((char*)&old_conf_data[0], len);
        UpdateConfigReqLog last_conf_log;
        if(!ReqLogMgr::unpackReqLogData(old_conf_data, last_conf_log))
        {
            forceExit("unpack last config data error");
        }
        last_conf.close();

        LOG(INFO) << "rename unchanged config and overwrite with last config";
        std::map<std::string, std::string>::const_iterator cit = cur_conf_log.config_file_list.begin();
        for(; cit != cur_conf_log.config_file_list.end(); ++cit)
        {
            if (bfs::exists(configDir_ + "/" + cit->first + ".unchanged"))
                bfs::remove(configDir_ + "/" + cit->first + ".unchanged");
            bfs::rename(configDir_ + "/" + cit->first, configDir_ + "/" + cit->first + ".unchanged");
        }
        running_col_info.clear();
        cit = last_conf_log.config_file_list.begin();
        for(; cit != last_conf_log.config_file_list.end(); ++cit)
        {
            LOG(INFO) << "using last config for : " << cit->first;
            std::ofstream ofs;
            ofs.open(std::string(configDir_ + "/" + cit->first).c_str(), ios::binary);
            ofs << cit->second << std::flush;
            ofs.close();
            running_col_info[cit->first.substr(0, cit->first.length() - 4)] = configDir_ + "/" + cit->first;
        }
    }
    else
    {
        std::string new_conf_data;
        LOG(INFO) << "using new config, replace old with new.";
        ReqLogMgr::packReqLogData(cur_conf_log, new_conf_data);
        std::ofstream ofs;
        ofs.open(last_conf_file_.c_str(), ios::binary);
        ofs << new_conf_data << std::flush;
        ofs.close();
    }
    return running_col_info;
}

void RecoveryChecker::checkDataConsistent(const std::string& coll, std::string& errinfo)
{
    if (flush_col_)
        flush_col_(coll);
    std::vector<std::string> coll_list;
    coll_list.push_back(coll);
    DistributeFileSyncMgr::get()->checkReplicasStatus(coll_list, errinfo);
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

    std::vector<std::string> coll_list;
    CollInfoMapT::const_iterator cit = tmp_all_col_info.begin();
    while(cit != tmp_all_col_info.end())
    {
        if (flush_col_)
            flush_col_(cit->first);
        coll_list.push_back(cit->first);
        ++cit;
    }
    DistributeFileSyncMgr::get()->checkReplicasStatus(coll_list, errinfo);
    if (!errinfo.empty())
    {
        LOG(ERROR) << "data is not consistent after recovery, error : " << errinfo;
        return false;
    }
    return true;
}

void RecoveryChecker::onRecoverWaitPrimaryCallback()
{
    LOG(INFO) << "primary agreed , sync new request druing waiting recovery.";
    // check new request during wait recovery.
    syncSCDFiles();
    // check data consistent with primary.
    //
    if (NodeManagerBase::isAsyncEnabled() && NodeManagerBase::get()->isOtherPrimaryAvailable() && !checkIfLogForward(false))
    {
        LOG(INFO) << "check log failed while recover, need rollback";
        if(!rollbackLastFail(false))
        {
            forceExit("rollback failed for forword log.");
        }
    }
    LOG(INFO) << "primary agreed and my recovery finished, begin enter cluster";
    syncToNewestReqLog();

    std::string errinfo;
    CollInfoMapT tmp_all_col_info;
    {
        boost::unique_lock<boost::mutex> lock(mutex_);
        tmp_all_col_info = all_col_info_;
    }
    bool sync_file = bfs::exists("./distribute_sync_file.flag");

    std::vector<std::string> coll_list;
    CollInfoMapT::const_iterator cit = tmp_all_col_info.begin();
    while(cit != tmp_all_col_info.end())
    {
        if (flush_col_)
            flush_col_(cit->first);
        coll_list.push_back(cit->first);
        ++cit;
    }

    DistributeFileSyncMgr::get()->checkReplicasStatus(coll_list, errinfo);
    if (!errinfo.empty())
    {
        setRollbackFlag(0);
        LOG(ERROR) << "data is not consistent after recovery, error : " << errinfo;
        if (sync_file)
        {
            if(!DistributeFileSyncMgr::get()->syncCollectionData(coll_list))
                forceExit("recovery failed for sync collection file.");
        }
        else
            forceExit("recovery failed for not consistent."); 
        clearRollbackFlag();
    }

    if (sync_file)
        bfs::remove("./distribute_sync_file.flag");

    if (need_backup_)
    {
        LOG(INFO) << "begin backup for config updated.";
        backup();
    }
}

void RecoveryChecker::onRecoverWaitReplicasCallback()
{
    LOG(INFO) << "all recovery replicas has entered the cluster after recovery finished.";
    LOG(INFO) << "wait recovery finished , primary ready to server";
    NodeManagerBase::get()->updateLastWriteReqId(reqlog_mgr_->getLastSuccessReqId());
    if (need_backup_)
    {
        LOG(INFO) << "begin backup for config updated.";
        backup();
    }
}

// check whether the current node log is newer than primary,
// If true, we need abandon the newer log and rollback.
bool RecoveryChecker::checkIfLogForward(bool is_primary)
{
    uint32_t newest_reqid = reqlog_mgr_->getLastSuccessReqId();
    LOG(INFO) << "starting last request is :" << newest_reqid;
    if (is_primary)
    {
        LOG(INFO) << "primary no need check log.";
        return true;
    }
    // check if log data is same or if log data is newer than current primary.
    // if primary is down and restart, some log may newer than current primary,
    // we should abandon these log and sync to new primary.
    uint32_t check_start = 0;
    std::vector<uint32_t> local_logid_list;
    std::vector<std::string> local_logdata_list;
    std::vector<std::string> primary_logdata_list;
    while(true)
    {
        LOG(INFO) << "checking log start from :" << check_start;
        if(!DistributeFileSyncMgr::get()->getNewestReqLog(true, check_start, primary_logdata_list))
        {
            if (!NodeManagerBase::get()->isConnected())
            {
                LOG(ERROR) << "zookeeper connection lost!!!";
                return false;
            }
            if (!NodeManagerBase::get()->isOtherPrimaryAvailable())
            {
                LOG(INFO) << "no other primary node while check log.";
                return true;
            }
            LOG(WARNING) << "get newest log from primary failed while checking log data. waiting and retry...";
            sleep(10);
            continue;
        }
        std::vector<uint32_t>().swap(local_logid_list);
        std::vector<std::string>().swap(local_logdata_list);
        reqlog_mgr_->getReqLogIdList(check_start, primary_logdata_list.size(), true, local_logid_list, local_logdata_list);

        size_t min_size = std::min(local_logdata_list.size(), primary_logdata_list.size());
        for (size_t i = 0; i < min_size; ++i)
        {
            if( local_logdata_list[i] != primary_logdata_list[i] )
            {
                LOG(INFO) << "current node log data diff from primary from : " << local_logid_list[i];
                break;
            }
            else
            {
                check_start = local_logid_list[i];
            }
        }

        if (check_start >= newest_reqid)
        {
            LOG(INFO) << "check log data success, current node log ok.";
            return true;
        }
        if (local_logid_list.empty() || check_start != local_logid_list.back())
        {
            LOG(INFO) << "the log data is not ok, need rollback to " << check_start;
            setRollbackFlag(check_start + 1);
            return false;
        }
        ++check_start;
    }
    return false;
}

bool RecoveryChecker::onRecoverCheckLog(bool is_primary)
{
    if (NodeManagerBase::isAsyncEnabled())
    {
        return checkIfLogForward(is_primary);
    }
    LOG(INFO) << "check log for re-connect.";
    uint32_t lastid = NodeManagerBase::get()->getLastWriteReqId();
    if (lastid == 0)
        return true;
    if (lastid == reqlog_mgr_->getLastSuccessReqId())
    {
        LOG(INFO) << "current last is same as zookeeper";
        return true;
    }
    else if (lastid > reqlog_mgr_->getLastSuccessReqId())
    {
        LOG(INFO) << "current node log fall behind, need sync to newest " << reqlog_mgr_->getLastSuccessReqId();
    }
    else
    {
        LOG(INFO) << "current node is forword, something error.";
        if (lastid + 1 == reqlog_mgr_->getLastSuccessReqId())
        {
            LOG(WARNING) << "last update write request id to zookeeper may lost. need re-update.";
            NodeManagerBase::get()->updateLastWriteReqId(reqlog_mgr_->getLastSuccessReqId());
            return true;
        }
    }
    return false;
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
        bool sync_from_primary = NodeManagerBase::isAsyncEnabled();
        while(!DistributeFileSyncMgr::get()->getNewestReqLog(sync_from_primary, reqid + 1, newlogdata_list))
        {
            if (!NodeManagerBase::get()->isConnected())
            {
                LOG(ERROR) << "zookeeper connection lost!!!";
                break;
            }
            if (!NodeManagerBase::get()->isOtherPrimaryAvailable())
            {
                LOG(INFO) << "no other primary node while sync log.";
                break;
            }
            LOG(INFO) << "get newest log failed, waiting and retry.";
            sleep(10);
        }
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
    if (!NodeManagerBase::get()->isDistributed())
        return true;
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

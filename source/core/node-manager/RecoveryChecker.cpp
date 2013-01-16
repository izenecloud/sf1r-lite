#include "RecoveryChecker.h"
#include "DistributeFileSyncMgr.h"
#include "DistributeDriver.h"
#include <boost/filesystem.hpp>
#include "RequestLog.h"
#include "SearchNodeManager.h"
#include <glog/logging.h>

#define MAX_BACKUP_NUM 3
namespace bfs = boost::filesystem;

namespace sf1r
{

void RecoveryChecker::cleanUnnessesaryBackup(const std::string& backup_basepath)
{
    // only keep the lastest 3 backups.
    std::vector<uint32_t> backup_req_incids;
    getBackupList(backup_basepath, backup_req_incids);
    for (size_t i = MAX_BACKUP_NUM; i < backup_req_incids.size(); ++i)
    {
        bfs::remove_all(backup_basepath + boost::lexical_cast<std::string>(backup_req_incids[i]));
    }
}

std::string RecoveryChecker::getRollbackFile()
{
    return "./rollback-flag";
}

bool RecoveryChecker::setRollbackFlag(uint32_t inc_id, const CollectionPath& colpath)
{
    std::string rollback_file = getRollbackFile();
    if (bfs::exists(rollback_file))
    {
        LOG(ERROR) << "rollback_file already exist, this mean last rollback is not finished. must exit.";
        throw -1;
    }
    ofstream ofs(rollback_file.c_str());
    if (ofs.good())
    {
        ofs.write((const char*)&inc_id, sizeof(inc_id));
        std::string backup_basepath = colpath.getBasePath() + "/req-backup/";
        const size_t &len = backup_basepath.size();
        ofs.write((const char*)&len, sizeof(len));
        ofs.write(backup_basepath.c_str(), backup_basepath.size());
        return true;
    }
    return false;
}

void RecoveryChecker::clearRollbackFlag()
{
    std::string rollback_file = getRollbackFile();
    if (bfs::exists(rollback_file))
        bfs::remove(rollback_file);
}

void RecoveryChecker::getBackupList(const std::string& backup_basepath, std::vector<uint32_t>& backup_req_incids)
{
    // only keep the lastest 3 backups.
    static bfs::directory_iterator end_dir = bfs::directory_iterator();
    bfs::directory_iterator backup_dirs_it = bfs::directory_iterator(bfs::path(backup_basepath));
    while(backup_dirs_it != end_dir)
    {
        if (!bfs::is_directory(*backup_dirs_it))
        {
            ++backup_dirs_it;
            continue;
        }
        try
        {
            uint32_t inc_id = boost::lexical_cast<uint32_t>((*backup_dirs_it).path().relative_path());
            backup_req_incids.push_back(inc_id);
        }
        catch(const boost::bad_lexical_cast& e)
        {
        }
        ++backup_dirs_it;
    }
    std::sort(backup_req_incids.begin(), backup_req_incids.end(), std::greater<uint32_t>());
}

bool RecoveryChecker::getLastBackup(const std::string& backup_basepath, std::string& backup_path, uint32_t& backup_inc_id)
{
    std::vector<uint32_t> backup_req_incids;
    getBackupList(backup_basepath, backup_req_incids);
    if (backup_req_incids.empty())
        return false;
    backup_inc_id = backup_req_incids.front();
    backup_path = backup_basepath + boost::lexical_cast<std::string>(backup_inc_id);
    return true;
}

bool RecoveryChecker::backup(const CollectionPath& colpath, ReqLogMgr* reqlogmgr)
{
    // backup changeable data first, so that we can rollback if old data corrupt while process the request.
    // Ignore SCD files and any other files that will not change during processing.
    std::string basepath = colpath.getBasePath() + "/req-backup/";
    std::string coldata_path = colpath.getCollectionDataPath();
    std::string querydata_path = colpath.getQueryDataPath();
    std::string req_log_path = reqlogmgr->getRequestLogPath();

    try
    {
        std::string dest_path = basepath + boost::lexical_cast<std::string>(reqlogmgr->getLastSuccessReqId()) + "/";
        boost::filesystem3::copy(coldata_path, dest_path + bfs::path(coldata_path).relative_path().c_str());
        boost::filesystem3::copy(querydata_path, dest_path + bfs::path(querydata_path).relative_path().c_str());
        boost::filesystem3::copy(req_log_path, dest_path + bfs::path(req_log_path).relative_path().c_str());
    }
    catch(const std::exception& e)
    {
        // clear state.
        std::cerr << "backup for write request failed. " << e.what() << std::endl;
        return false;
    }
    cleanUnnessesaryBackup(basepath);
    return true;
}

// note : if there are more than one collection we need backup
// all collection data. only in this way we can rollback and redo log to 
// rollback the data before last failed request.
bool RecoveryChecker::rollbackLastFail(ReqLogMgr* reqlogmgr)
{
    // not all inc_id has a backup, so we just find the newest backup.
    std::string rollback_file = getRollbackFile();
    if (!bfs::exists(rollback_file))
    {
        LOG(INFO) << "rollback flag not exist, no need to rollback.";
        return true;
    }
    ifstream ifs(rollback_file.c_str());
    if (!ifs.good())
    {
        LOG(ERROR) << "read rollback flag file error.";
        return false;
    }
    uint32_t rollback_id;
    size_t len;
    std::string rollback_backup_basepath;
    ifs.read((char*)&rollback_id, sizeof(rollback_id));
    ifs.read((char*)&len, sizeof(len));
    rollback_backup_basepath.resize(len);
    ifs.read(&rollback_backup_basepath[0], len);

    LOG(INFO) << "last failed request inc_id is :" << rollback_id;
    std::string last_backup_path;
    uint32_t last_backup_id;
    std::string req_log_path = reqlogmgr->getRequestLogPath();
    uint32_t  last_write_id = reqlogmgr->getLastSuccessReqId();
    // mv current log to redo log, copy backup log to current, 
    bfs::rename(req_log_path, "./redo-reqlog/");
    ReqLogMgr redo_req_log_mgr;
    redo_req_log_mgr.init("./redo-reqlog/");
    std::string colname = "test";
    // copy backup data to replace current and reopen all file.
    if (!getLastBackup(rollback_backup_basepath, last_backup_path, last_backup_id))
    {
        LOG(ERROR) << "no backup available while rollback.";
        LOG(ERROR) << "need restart to redo all log or get full data from primary.";
        last_backup_id = 0;
        stop_clean_col_(colname);
        return false;
    }
    else
    {
        // copy backup data to replace current and reopen all file.
        stop_col_(colname);
        boost::filesystem3::copy(last_backup_path, bfs::path(last_backup_path)/bfs::path("..")/bfs::path(".."));
    }
    reqlogmgr->init(req_log_path);
    assert(last_backup_id < rollback_id);
    LOG(INFO) << "rollback from " << rollback_id << " to backup: " << last_backup_id;
    start_col_(colname, colname + ".xml");
    if (last_backup_id == last_write_id)
    {
        LOG(INFO) << "last backup is up to newest log. just replace data with backup.";
    }
    else
    {
        LOG(INFO) << "last backup is out of date. replace data with backup and redo request.";
        // read redo log and do request to current log.
        last_backup_id++;
        ReqLogHead rethead;
        size_t redo_offset;
        redo_req_log_mgr.getHeadOffset(last_backup_id, rethead, redo_offset);
        while(true)
        {
            std::string req_packed_data; 
            bool hasmore = redo_req_log_mgr.getReqDataByHeadOffset(redo_offset, rethead, req_packed_data);
            if (!hasmore)
                break;
            LOG(INFO) << "redoing for request id : " << rethead.inc_id;
            CommonReqData req_commondata;
            ReqLogMgr::unpackReqLogData(req_packed_data, req_commondata);
            if(!DistributeDriver::get()->handleReqFromLog(req_commondata.req_json_data, req_packed_data))
            {
                LOG(INFO) << "redoing from log failed: " << rethead.inc_id;
                return false;
            }
        }
    }
    clearRollbackFlag();
    LOG(INFO) << "rollback to last failed finished.";
    return true;
}

void RecoveryChecker::init()
{
    SearchNodeManager::get()->setRecoveryCallback(
        boost::bind(&RecoveryChecker::onRecoverCallback, this),
        boost::bind(&RecoveryChecker::onRecoverWaitPrimaryCallback, this),
        boost::bind(&RecoveryChecker::onRecoverWaitReplicasCallback, this));
}

void RecoveryChecker::onRecoverCallback()
{
    LOG(INFO) << "recovery callback, begin recovery before enter cluster.";
    syncToNewestReqLog();
    // if failed, must exit.
    LOG(INFO) << "recovery finished, wait primary agree before enter cluster.";
}

void RecoveryChecker::onRecoverWaitPrimaryCallback()
{
    LOG(INFO) << "primary agreed my recovery finished, begin enter cluster";
}

void RecoveryChecker::onRecoverWaitReplicasCallback()
{
    LOG(INFO) << "wait replica enter cluster after recovery finished.";
}

void RecoveryChecker::syncToNewestReqLog()
{
    LOG(INFO) << "begin sync to newest log.";
    while(true)
    {
        DistributeFileSyncMgr::get()->getNewestReqLog(0, "./redo-reqlog");
        // redo new request from newest log file.
        // 
        // if no any new request break while.
    }
}

void RecoveryChecker::wait()
{
}

}

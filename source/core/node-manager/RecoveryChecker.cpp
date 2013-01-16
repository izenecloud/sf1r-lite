#include "RecoveryChecker.h"
#include "DistributeFileSyncMgr.h"
#include "DistributeDriver.h"
#include <boost/filesystem.hpp>
#include "RequestLog.h"
#include "SearchNodeManager.h"
#include <process/common/XmlConfigParser.h>

#include <glog/logging.h>

#define MAX_BACKUP_NUM 3
namespace bfs = boost::filesystem;

namespace sf1r
{

void RecoveryChecker::updateCollection(const SF1Config& sf1_config)
{
    all_col_info_.clear();
    SF1Config::CollectionMetaMap::const_iterator cit = sf1_config.getCollectionMetaMap().begin();
    while(cit != sf1_config.getCollectionMetaMap().end())
    {
        all_col_info_[cit->first] = std::make_pair(cit->second.getCollectionPath(),
            sf1_config.getCollectionConfigFile(cit->first));
        ++cit;
    }
}

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

bool RecoveryChecker::setRollbackFlag(uint32_t inc_id)
{
    if (bfs::exists(rollback_file_))
    {
        LOG(ERROR) << "rollback_file already exist, this mean last rollback is not finished. must exit.";
        throw -1;
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

bool RecoveryChecker::backup()
{
    // backup changeable data first, so that we can rollback if old data corrupt while process the request.
    // Ignore SCD files and any other files that will not change during processing.

    // find all collection and backup.
    CollInfoMapT::const_iterator cit = all_col_info_.begin();
    std::string dest_path = backup_basepath_ + boost::lexical_cast<std::string>(reqlog_mgr_->getLastSuccessReqId()) + "/";
    while(cit != all_col_info_.end())
    {
        LOG(INFO) << "backing up the collection: " << cit->first;
        std::string dest_col_backup = dest_path + cit->first + "_backup/";
        const CollectionPath &colpath = cit->second.first;

        std::string col_basepath = colpath.getBasePath();
        std::string coldata_path = colpath.getCollectionDataPath();
        std::string querydata_path = colpath.getQueryDataPath();

        try
        {
            boost::filesystem3::copy(col_basepath + coldata_path, dest_col_backup + coldata_path);
            boost::filesystem3::copy(col_basepath + querydata_path, dest_col_backup + querydata_path);
        }
        catch(const std::exception& e)
        {
            // clear state.
            LOG(ERROR) << "backup collection data failed. " << e.what() << std::endl;
            return false;
        }
        ++cit;
    }
    try
    {
        boost::filesystem3::copy(request_log_basepath_, dest_path + bfs::path(request_log_basepath_).relative_path().c_str());
    }
    catch(const std::exception& e)
    {
        // clear state.
        LOG(ERROR) << "backup request log failed. " << e.what() << std::endl;
        return false;
    }
    cleanUnnessesaryBackup(backup_basepath_);
    return true;
}

// note : if there are more than one collection we need backup
// all collection data. only in this way we can rollback and redo log to 
// rollback the data before last failed request.
bool RecoveryChecker::rollbackLastFail()
{
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
    uint32_t last_backup_id;
    uint32_t  last_write_id = reqlog_mgr_->getLastSuccessReqId();
    // mv current log to redo log, copy backup log to current, 
    bfs::rename(request_log_basepath_, redo_log_basepath_);
    ReqLogMgr redo_req_log_mgr;
    redo_req_log_mgr.init(redo_log_basepath_);
    if (!getLastBackup(backup_basepath_, last_backup_path, last_backup_id))
    {
        LOG(ERROR) << "no backup available while rollback.";
        LOG(ERROR) << "need restart to redo all log or get full data from primary.";
        // remove all data and redo all log.
        last_backup_id = 0;
    }

    // stop collection will change the metamap so we need a copy. 
    CollInfoMapT tmp_all_col_info = all_col_info_;
    CollInfoMapT::const_iterator cit = tmp_all_col_info.begin();
    while(cit != tmp_all_col_info.end())
    {
        stop_col_(cit->first, last_backup_id == 0);
        ++cit;
    }

    // copy backup data to replace current and reopen all file.
    if (last_backup_id > 0)
    {
        try
        {
            // copy backup log to current .
            boost::filesystem3::copy(last_backup_path + bfs::path(request_log_basepath_).relative_path().c_str(),
                request_log_basepath_);
            CollInfoMapT::const_iterator inner_cit = tmp_all_col_info.begin();
            while(inner_cit != tmp_all_col_info.end())
            {
                LOG(INFO) << "restoring the backup for the collection: " << inner_cit->first;
                std::string dest_col_backup = last_backup_path + cit->first + "_backup/";
                const CollectionPath &colpath = inner_cit->second.first;

                std::string col_basepath = colpath.getBasePath();
                std::string coldata_path = colpath.getCollectionDataPath();
                std::string querydata_path = colpath.getQueryDataPath();

                boost::filesystem3::copy(dest_col_backup + coldata_path, col_basepath + coldata_path);
                boost::filesystem3::copy(dest_col_backup + querydata_path, col_basepath + querydata_path);
                ++inner_cit;
            }
        }
        catch(const std::exception& e)
        {
            LOG(ERROR) << "copy backup to current working path failed." << e.what();
            bfs::rename(redo_log_basepath_, request_log_basepath_);
            return false;
        }
    }

    assert(last_backup_id < rollback_id);
    LOG(INFO) << "rollback from " << rollback_id << " to backup: " << last_backup_id;
    reqlog_mgr_->init(request_log_basepath_);

    clearRollbackFlag();
    LOG(INFO) << "rollback to last backup finished, begin redo from backup to sync to last fail request.";
    cit = tmp_all_col_info.begin();
    while(cit != tmp_all_col_info.end())
    {
        if(!start_col_(cit->first, cit->second.second))
            return false;
        ++cit;
    }

    if (last_backup_id == last_write_id)
    {
        LOG(INFO) << "last backup is up to newest log. no redo need.";
    }
    else
    {
        LOG(INFO) << "last backup is out of date. begin redo request.";
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
    bfs::remove_all(redo_log_basepath_);
    return true;
}

void RecoveryChecker::init(const std::string& workdir)
{
    backup_basepath_ = workdir + "/req-backup/";
    request_log_basepath_ = workdir + "/req-log/";
    redo_log_basepath_ = workdir + "/redo-log/";
    rollback_file_ = workdir + "/rollback_flag";

    reqlog_mgr_.reset(new ReqLogMgr());
    reqlog_mgr_->init(request_log_basepath_);

    SearchNodeManager::get()->setRecoveryCallback(
        boost::bind(&RecoveryChecker::onRecoverCallback, this),
        boost::bind(&RecoveryChecker::onRecoverWaitPrimaryCallback, this),
        boost::bind(&RecoveryChecker::onRecoverWaitReplicasCallback, this));
}

void RecoveryChecker::onRecoverCallback()
{
    LOG(INFO) << "recovery callback, begin recovery before enter cluster.";
    rollbackLastFail();
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
        DistributeFileSyncMgr::get()->getNewestReqLog(0, redo_log_basepath_);
        // redo new request from newest log file.
        // 
        // if no any new request break while.
    }
}

void RecoveryChecker::wait()
{
}

}

#include "DistributeRequestHooker.h"
#include "DistributeDriver.h"
#include <boost/filesystem.hpp>
#include <util/driver/writers/JsonWriter.h>
#include <util/driver/readers/JsonReader.h>
#include <bundles/index/IndexBundleConfiguration.h>
#include <node-manager/SearchNodeManager.h>

#include "RequestLog.h"

#define MAX_BACKUP_NUM 3

using namespace izenelib::driver;
using namespace izenelib;
namespace bfs = boost::filesystem;

namespace sf1r
{

std::set<ReqLogType> DistributeRequestHooker::need_backup_types_;

void DistributeRequestHooker::init()
{
    need_backup_types_.insert(Req_Index);
    need_backup_types_.insert(Req_CreateDoc);
}

std::string DistributeRequestHooker::getRollbackFile(const CollectionPath& colpath, uint32_t inc_id)
{
    return colpath.getBasePath() + "/rollback-flag-" + boost::lexical_cast<std::string>(inc_id);
}

void DistributeRequestHooker::getBackupList(const CollectionPath& colpath, std::vector<uint32_t>& backup_req_incids)
{
    // only keep the lastest 3 backups.
    std::string backup_basepath = colpath.getBasePath() + "/req-backup/";
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

bool DistributeRequestHooker::getLastBackup(const CollectionPath& colpath, std::string& backup_path, uint32_t& backup_inc_id)
{
    std::vector<uint32_t> backup_req_incids;
    getBackupList(colpath, backup_req_incids);
    if (backup_req_incids.empty())
        return false;
    backup_inc_id = backup_req_incids.front();
    backup_path = colpath.getBasePath() + "/req-backup/" + boost::lexical_cast<std::string>(backup_inc_id);
    return true;
}

DistributeRequestHooker::DistributeRequestHooker()
    :type_(Req_None), hook_type_(0)
{
    // init callback for distribute request.
    SearchNodeManager::get()->setCallback(
        boost::bind(&DistributeRequestHooker::onElectingFinished, this),
        boost::bind(&DistributeRequestHooker::waitReplicasProcessCallback, this),
        boost::bind(&DistributeRequestHooker::waitReplicasLogCallback, this),
        boost::bind(&DistributeRequestHooker::waitPrimaryCallback, this),
        boost::bind(&DistributeRequestHooker::abortRequestCallback, this),
        boost::bind(&DistributeRequestHooker::waitReplicasAbortCallback, this),
        boost::bind(&DistributeRequestHooker::onRequestFromPrimary, this, _1, _2));
}

bool DistributeRequestHooker::isNeedBackup(ReqLogType type)
{
    return need_backup_types_.find(type) != need_backup_types_.end();
}

void DistributeRequestHooker::hookCurrentReq(const std::string& colname, const CollectionPath& colpath,
    const std::string& reqdata,
    boost::shared_ptr<ReqLogMgr> req_log_mgr)
{
    colname_ = colname;
    colpath_ = colpath;
    current_req_ = reqdata;
    req_log_mgr_ = req_log_mgr;
}

void DistributeRequestHooker::onRequestFromPrimary(int type, const std::string& packed_reqdata)
{
    LOG(INFO) << "callback for new request from primary";
    CommonReqData reqloghead;
    if(!ReqLogMgr::unpackReqLogData(packed_reqdata, reqloghead))
    {
        LOG(ERROR) << "unpack request data from primary failed.";
        abortRequest();
        return;
    }
    DistributeDriver::get()->handleReqFromPrimary(reqloghead.req_json_data, packed_reqdata);
}

void DistributeRequestHooker::setHook(int calltype, const std::string& addition_data)
{
    hook_type_ = calltype;
    // for request for primary master, the addition_data is the original request json data.
    // for request from primary worker to replicas, the addition_data is original request
    // json data plus the data used for this request.
    primary_addition_ = addition_data;
}

int  DistributeRequestHooker::getHookType()
{
    return hook_type_;
}

bool DistributeRequestHooker::isHooked()
{
    return (hook_type_ > 0) && !current_req_.empty();
}

void DistributeRequestHooker::cleanUnnessesaryBackup()
{
    // only keep the lastest 3 backups.
    std::string backup_path = colpath_.getBasePath() + "/req-backup/";
    std::vector<uint32_t> backup_req_incids;
    getBackupList(colpath_, backup_req_incids);
    for (size_t i = MAX_BACKUP_NUM; i < backup_req_incids.size(); ++i)
    {
        bfs::remove_all(backup_path + boost::lexical_cast<std::string>(backup_req_incids[i]));
    }
}

bool DistributeRequestHooker::prepare(ReqLogType type, CommonReqData& prepared_req)
{
    if (!isHooked())
        return true;
    if (!req_log_mgr_)
        return false;
    bool isprimary = SearchNodeManager::get()->isPrimary();
    if (isprimary)
        prepared_req.req_json_data = current_req_;
    else
    {
        // get addition data from primary
        CommonReqData reqloghead;
        if(!ReqLogMgr::unpackReqLogData(current_req_, reqloghead))
        {
            LOG(ERROR) << "unpack log data failed while prepare the data from primary.";
            forceExit();
        }
        if (type != (ReqLogType)reqloghead.reqtype)
        {
            LOG(ERROR) << "log type mismatch with primary while prepare the data from primary.";
            LOG(ERROR) << "It may happen when the code is not the same. Must exit.";
            forceExit();
        }
        prepared_req.inc_id = reqloghead.inc_id;
        prepared_req.req_json_data = reqloghead.req_json_data;
        LOG(INFO) << "got create document request from primary, inc_id :" << prepared_req.inc_id;
    }
    prepared_req.reqtype = type;
    type_ = type;
    if (!req_log_mgr_->prepareReqLog(prepared_req, isprimary))
    {
        return false;
    }
    std::string basepath = colpath_.getBasePath();
    
    if (isNeedBackup(type))
    {
        // backup changeable data first, so that we can rollback if old data corrupt while process the request.
        // Ignore SCD files and any other files that will not change during processing.
        std::string coldata_path = colpath_.getCollectionDataPath();
        std::string querydata_path = colpath_.getQueryDataPath();

        try
        {
            boost::filesystem3::copy(coldata_path, basepath + "/req-backup/" +
                boost::lexical_cast<std::string>(req_log_mgr_->getLastSuccessReqId()) + "/");
            boost::filesystem3::copy(querydata_path, basepath + "/req-backup/" +
                boost::lexical_cast<std::string>(req_log_mgr_->getLastSuccessReqId()) + "/");
        }
        catch(const std::exception& e)
        {
            // clear state.
            std::cerr << "backup for write request failed. " << e.what() << std::endl;
            abortRequest();
            return false;
        }
        cleanUnnessesaryBackup();
    }
    // set rollback flag.
    if(!setRollbackFlag(prepared_req.inc_id))
    {
        LOG(ERROR) << "set rollback failed.";
        abortRequest();
        return false; 
    }
    return true;
}

void DistributeRequestHooker::processLocalBegin()
{
    if (!isHooked())
        return;
    LOG(INFO) << "begin process request on worker";
    SearchNodeManager::get()->beginReqProcess();
}

void DistributeRequestHooker::processLocalFinished(bool finishsuccess, const std::string& packed_req_data)
{
    if (!isHooked())
        return;
    LOG(INFO) << "begin process request on local worker finished.";
    current_req_ = packed_req_data;
    if (!finishsuccess)
    {
        LOG(INFO) << "process finished fail.";
        abortRequest();
        return;
    }
    SearchNodeManager::get()->finishLocalReqProcess(type_, packed_req_data);
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
    SearchNodeManager::get()->abortRequest();
}

bool DistributeRequestHooker::setRollbackFlag(uint32_t inc_id)
{
    std::string rollback_file = getRollbackFile(colpath_, inc_id);
    ofstream ofs(rollback_file.c_str());
    if (ofs.good())
    {
        ofs.write((const char*)&inc_id, sizeof(inc_id));
        return true;
    }
    return false;
}

void DistributeRequestHooker::clearRollbackFlag(uint32_t inc_id)
{
    std::string rollback_file = getRollbackFile(colpath_, inc_id);
    if (bfs::exists(rollback_file))
        bfs::remove(rollback_file);
}

void DistributeRequestHooker::abortRequestCallback()
{
    if (!isHooked())
        return;
    LOG(INFO) << "got abort request.";
    CommonReqData prepared_req;
    if (!req_log_mgr_->getPreparedReqLog(prepared_req))
    {
        LOG(ERROR) << "get prepare request log data failed while abort request.";
        forceExit();
    }
    finish(false, prepared_req);
}

void DistributeRequestHooker::waitReplicasAbortCallback()
{
    if (!isHooked())
        return;
    LOG(INFO) << "all replicas finished the abort request.";
}

void DistributeRequestHooker::writeLocalLog()
{
    if (!isHooked())
        return;

    LOG(INFO) << "begin write request log to local.";
    CommonReqData prepared_req;
    if (!req_log_mgr_->getPreparedReqLog(prepared_req))
    {
        LOG(ERROR) << "get prepare request log data failed.";
        forceExit();
    }

    bool ret = req_log_mgr_->appendReqData(current_req_);
    if (!ret)
    {
        LOG(ERROR) << "write local log failed. something wrong on this node, must exit.";
        forceExit();
    }
    finish(true, prepared_req);
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
    if(!isHooked())
        return;
    LOG(INFO) << "an electing has finished. notify ready to master.";
}

void DistributeRequestHooker::finish(bool success, const CommonReqData& prepared_req)
{
    try
    {
        if (success)
        {
            LOG(INFO) << "The request has finally finished both on primary and replicas.";
        }
        else
        {
            LOG(INFO) << "The request failed to finish. rollback from backup.";
            // rollback from backup.
            // rename current and move restore.
            // all the file need to be reopened to make effective.
            std::string last_backup_path;
            uint32_t last_backup_id;
            if (!getLastBackup(colpath_, last_backup_path, last_backup_id))
            {
                LOG(ERROR) << "no backup available while rollback.";
                forceExit();
            }
            if (last_backup_id > req_log_mgr_->getLastSuccessReqId())
            {
                LOG(ERROR) << "wrong data, this node is unrecoverable.";
                forceExit();
            }
            else if (last_backup_id == req_log_mgr_->getLastSuccessReqId())
            {
                LOG(INFO) << "rollback to last backup and no log redo needed.";
            }
            else
            {
                LOG(INFO) << "backup is not the newest, log redo needed";
                LOG(INFO) << "must restart to begin recovery process";
                forceExit();
            }
        }
    }
    catch(const std::exception& e)
    {
        LOG(ERROR) << "failed finish. must exit.";
        forceExit();
    }
    clearRollbackFlag(prepared_req.inc_id);
    req_log_mgr_->delPreparedReqLog();
    current_req_.clear();
    req_log_mgr_.reset();
    hook_type_ = 0;
    primary_addition_.clear();
}

void DistributeRequestHooker::forceExit()
{
    throw -1;
}

}


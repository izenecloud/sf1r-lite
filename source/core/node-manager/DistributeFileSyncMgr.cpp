#include "DistributeFileSyncMgr.h"
#include "SearchNodeManager.h"
#include "SuperNodeManager.h"
#include "RequestLog.h"
#include "RecoveryChecker.h"
#include <net/distribute/DataTransfer2.hpp>
#include <configuration-manager/CollectionPath.h>
#include <sf1r-net/RpcServerConnection.h>
#include <iostream>
#include <boost/shared_ptr.hpp>
#include <glog/logging.h>
#include <boost/filesystem.hpp>

namespace bfs = boost::filesystem;
namespace ba = boost::asio;

namespace sf1r
{

static void doTransferFile(const ReadyReceiveData& reqdata)
{
    izenelib::net::distribute::DataTransfer2 transfer(reqdata.receiver_ip, reqdata.receiver_port);
    FinishReceiveRequest req;
    if (not transfer.syncSend(reqdata.filepath, reqdata.filepath, false))
    {
        LOG(INFO) << "transfer file failed: " << reqdata.filepath;
        req.param_.success = false;
    }
    else
    {
        LOG(INFO) << "transfer file success: " << reqdata.filepath;
        req.param_.success = true;
    }
    req.param_.filepath = reqdata.filepath;
    DistributeFileSyncMgr::get()->sendFinishNotifyToReceiver(reqdata.receiver_rpcip, reqdata.receiver_rpcport, req);
}

FileSyncServer::FileSyncServer(const std::string& host, uint16_t port, uint32_t threadNum)
    : host_(host)
    , port_(port)
    , threadNum_(threadNum)
    , threadpool_(threadNum_)
{
}

FileSyncServer::~FileSyncServer()
{
    std::cout << "~FileSyncServer()" << std::endl;
    stop();
}

void FileSyncServer::start()
{
    instance.listen(host_, port_);
    instance.start(threadNum_);
}

void FileSyncServer::join()
{
    instance.join();
}

void FileSyncServer::run()
{
    start();
    join();
}

void FileSyncServer::stop()
{
    instance.end();
    instance.join();
}

void FileSyncServer::dispatch(msgpack::rpc::request req)
{
    try
    {
        std::string method;
        req.method().convert(&method);

        if (method == FileSyncServerRequest::method_names[FileSyncServerRequest::METHOD_TEST])
        {
            msgpack::type::tuple<bool> params;
            req.params().convert(&params);
            req.result(true);
        }
        else if (method == FileSyncServerRequest::method_names[FileSyncServerRequest::METHOD_GET_REQLOG])
        {
            msgpack::type::tuple<GetReqLogData> params;
            req.params().convert(&params);
            GetReqLogData& reqdata = params.get<0>();
            // get 100 new logs at most each time
            //
            reqdata.success = false;
            boost::shared_ptr<ReqLogMgr> reqlogmgr = RecoveryChecker::get()->getReqLogMgr();
            ReqLogHead head;
            size_t headoffset;
            std::string req_packed_data;
            bool ret = reqlogmgr->getHeadOffset(reqdata.start_inc, head, headoffset);
            if (!ret)
            {
                LOG(INFO) << "no more log after inc_id : " << reqdata.start_inc;
            }
            else
            {
                reqdata.logdata_list.reserve(100);
                LOG(INFO) << "get log from inc_id : " << reqdata.start_inc;
                while(true)
                {
                    ret = reqlogmgr->getReqDataByHeadOffset(headoffset, head, req_packed_data);
                    if(!ret || reqdata.logdata_list.size() > 100)
                        break;
                    reqdata.logdata_list.push_back(req_packed_data);
                    reqdata.end_inc = head.inc_id;
                }
                LOG(INFO) << "get log last inc_id :" << head.inc_id;
            }
            
            reqdata.success = true;
            req.result(reqdata);
        }
        else if (method == FileSyncServerRequest::method_names[FileSyncServerRequest::METHOD_GET_SCD_LIST])
        {
            msgpack::type::tuple<GetSCDListData> params;
            req.params().convert(&params);
            GetSCDListData& reqdata = params.get<0>();
            // get all the scd file in the backup directory.
            reqdata.success = false;
            CollectionPath colpath;
            bool ret = RecoveryChecker::get()->getCollPath(reqdata.collection, colpath);
            if (ret)
            {
                reqdata.success = true;
                bfs::path backup_path = colpath.getScdPath() + "/index/backup/";
                if (bfs::exists(backup_path))
                {
                    static bfs::directory_iterator end_dir = bfs::directory_iterator();
                    bfs::directory_iterator backup_dirs_it = bfs::directory_iterator(backup_path);
                    while (backup_dirs_it != end_dir)
                    {
                        if (bfs::is_regular_file(*backup_dirs_it))
                        {
                            reqdata.scd_list.push_back((*backup_dirs_it).path().string());
                            LOG(INFO) << "find backup scd file : " << (*backup_dirs_it).path().string();
                        }
                        ++backup_dirs_it;
                    }
                }
            }
            req.result(reqdata);
        }
        else if (method == FileSyncServerRequest::method_names[FileSyncServerRequest::METHOD_GET_FILE])
        {
            msgpack::type::tuple<GetFileData> params;
            req.params().convert(&params);
            GetFileData& reqdata = params.get<0>();
            reqdata.success = false;
            // get the file info 
            if (bfs::exists(reqdata.filepath) && bfs::is_regular_file(reqdata.filepath))
            {
                reqdata.success = true;
                reqdata.filesize = bfs::file_size(reqdata.filepath);
            }
            req.result(reqdata);
        }
        else if (method == FileSyncServerRequest::method_names[FileSyncServerRequest::METHOD_READY_RECEIVE])
        {
            msgpack::type::tuple<ReadyReceiveData> params;
            req.params().convert(&params);
            ReadyReceiveData& reqdata = params.get<0>();
            reqdata.success = true;
            // start transfer thread.
            threadpool_.schedule(boost::bind(&doTransferFile, reqdata));
            req.result(reqdata);
        }
        else if (method == FileSyncServerRequest::method_names[FileSyncServerRequest::METHOD_FINISH_RECEIVE])
        {
            msgpack::type::tuple<FinishReceiveData> params;
            req.params().convert(&params);
            FinishReceiveData& reqdata = params.get<0>();
            reqdata.success = true;
            DistributeFileSyncMgr::get()->notifyFinishReceive(reqdata.filepath);
            req.result(reqdata);
        }
        else
        {
            req.error(msgpack::rpc::NO_METHOD_ERROR);
        }
    }
    catch (const msgpack::type_error& e)
    {
        req.error(msgpack::rpc::ARGUMENT_ERROR);
    }
    catch (const std::exception& e)
    {
        req.error(std::string(e.what()));
    }
}

DistributeFileSyncMgr::DistributeFileSyncMgr()
{
    RpcServerConnection *conn_mgr_ = new RpcServerConnection();
    RpcServerConnectionConfig config;
    config.rpcThreadNum = 4;
    conn_mgr_->init(config);
}

void DistributeFileSyncMgr::init()
{
    transfer_rpcserver_.reset(new FileSyncServer(SuperNodeManager::get()->getLocalHostIP(),
            SuperNodeManager::get()->getFileSyncRpcPort(), 4));
    transfer_rpcserver_->start();
}

DistributeFileSyncMgr::~DistributeFileSyncMgr()
{
    delete conn_mgr_;
    if (transfer_rpcserver_)
        transfer_rpcserver_->stop();
}

bool DistributeFileSyncMgr::getNewestReqLog(uint32_t start_from, std::vector<std::string>& saved_log)
{
    // note : for optimize, we can get log from any node that has entered the cluster and 
    // current state is ready.
    int retry = 3;

    srand( time(NULL) );

    while(retry-- > 0)
    {
        std::string ip;
        // because all replica node should have the same config,
        // so the rpc ports for all file sync servers are the same.
        uint16_t port = SuperNodeManager::get()->getFileSyncRpcPort();

        if(!SearchNodeManager::get()->getCurrNodeSyncServerInfo(ip, rand()))
        {
            LOG(ERROR) << "get file sync server failed.";
            return false;
        }
        LOG(INFO) << "try get newest log from: " << ip << ":" << port;
        GetReqLogRequest req;
        req.param_.start_inc = start_from;
        GetReqLogData rsp;

        try
        {
            conn_mgr_->syncRequest(ip, port, req, rsp);
        }
        catch(const std::exception& e)
        {
            LOG(INFO) << "send request error, will retry : " << e.what();
            continue;
        }
        if (rsp.success)
        {
            saved_log.swap(rsp.logdata_list);
            return true;
        }
    }
    LOG(INFO) << "get newest log data failed .";
    return false;
}

bool DistributeFileSyncMgr::syncNewestSCDFileList(const std::string& colname)
{
    // get the backup scd file list used for index.
    
    // get primary file sync ip:port
    //
    // send rpc request to get scd filelist
    //
    // receiver response that will contain the filelist.
    // for each file in list, send rpc request to get file info,
    // if file is the same as local, we think it already exist and
    // continue to get next.
    int retry = 3;
    srand( time(NULL) );

    while(retry-- > 0)
    {
        std::string ip;
        uint16_t port = SuperNodeManager::get()->getFileSyncRpcPort();
        if(!SearchNodeManager::get()->getCurrNodeSyncServerInfo(ip, rand()))
        {
            LOG(ERROR) << "get file sync server failed.";
            return false;
        }
        LOG(INFO) << "try get scd file list from: " << ip << ":" << port;
        GetSCDListRequest req;
        req.param_.collection = colname;
        GetSCDListData rsp;
        try
        {
            conn_mgr_->syncRequest(ip, port, req, rsp);
        }
        catch(const std::exception& e)
        {
            LOG(INFO) << "send request error, will retry : " << e.what();
            continue;
        }
        if (rsp.success)
        {
            for (size_t i = 0; i < rsp.scd_list.size(); ++i)
            {
                GetFileRequest file_req;
                file_req.param_.filepath = rsp.scd_list[i];
                GetFileData file_rsp;
                try
                {
                    conn_mgr_->syncRequest(ip, port, file_req, file_rsp);
                }
                catch(const std::exception& e)
                {
                    LOG(INFO) << "send request error, will retry : " << e.what();
                    break;
                }
                if (!rsp.success)
                {
                    LOG(INFO) << "get file data info failed, retry next.";
                    break;
                }
                if (bfs::exists(file_rsp.filepath))
                {
                    if (!bfs::is_regular_file(file_rsp.filepath))
                    {
                        LOG(INFO) << "found a file with same name but not a scd file.";
                        bfs::rename(file_rsp.filepath, file_rsp.filepath + "_renamed");
                    }
                    else if(bfs::file_size(file_rsp.filepath) == file_rsp.filesize)
                    {
                        LOG(INFO) << "local scd file is the same size , no need copy: " << file_rsp.filepath;
                        continue;
                    }
                }
                if(!getFileFromOther(ip, port, file_rsp.filepath, file_rsp.filesize))
                {
                    LOG(INFO) << "get file from other failed, retry next.";
                    break;
                }
                LOG(INFO) << "a scd file finished :" << file_rsp.filepath;
                if (i == rsp.scd_list.size() - 1)
                    return true;
            }
        }
    }
    LOG(INFO) << "sync to newest scd file list failed.";
    return false;
}

bool DistributeFileSyncMgr::getFileFromOther(const std::string& filepath)
{
    return false;
}

bool DistributeFileSyncMgr::getFileFromOther(const std::string& ip, uint16_t port,
    const std::string& filepath, uint64_t filesize)
{
    if (!transfer_rpcserver_)
        return false;
    ReadyReceiveRequest req;
    req.param_.receiver_ip = SuperNodeManager::get()->getLocalHostIP();
    req.param_.receiver_port = SuperNodeManager::get()->getDataReceiverPort();
    req.param_.receiver_rpcip = transfer_rpcserver_->getHost();
    req.param_.receiver_rpcport = transfer_rpcserver_->getPort();
    req.param_.filepath = filepath;
    req.param_.filesize = filesize;

    ReadyReceiveData rsp;
    try
    {
        conn_mgr_->syncRequest(ip, port, req, rsp);
    }
    catch(const std::exception& e)
    {
        LOG(INFO) << "send request error while get file from other: " << e.what();
        return false;
    }

    // wait receiver.
    return waitFinishReceive(filepath, filesize);
}

void DistributeFileSyncMgr::sendFinishNotifyToReceiver(const std::string& ip, uint16_t port, const FinishReceiveRequest& req)
{
    FinishReceiveData rsp;
    try
    {
        conn_mgr_->syncRequest(ip, port, req, rsp);
    }
    catch(const std::exception& e)
    {
        LOG(ERROR) << "send request error while notify finish to other: " << e.what();
    }
}

void DistributeFileSyncMgr::notifyFinishReceive(const std::string& filepath)
{
    boost::unique_lock<boost::mutex> lk(mutex_);
    if (wait_finish_notify_.find(filepath) == wait_finish_notify_.end())
    {
        LOG(INFO) << "a file finish notify but not waiting: " << filepath;
        return;
    }
    wait_finish_notify_[filepath] = true;
    cond_.notify_all();
    LOG(INFO) << "a file finish notify for : " << filepath;
}

bool DistributeFileSyncMgr::waitFinishReceive(const std::string& filepath, uint64_t filesize)
{
    boost::unique_lock<boost::mutex> lk(mutex_);
    if (wait_finish_notify_.find(filepath) != wait_finish_notify_.end())
    {
        LOG(INFO) << "file receiver is already waiting : " << filepath;
        return false;
    }
    wait_finish_notify_[filepath] = false;
    while(!wait_finish_notify_[filepath])
    {
        cond_.wait(lk);
    }
    LOG(INFO) << "a file finished receive : " << filepath;
    wait_finish_notify_.erase(filepath);
    if (bfs::exists(filepath) && bfs::is_regular_file(filepath) && bfs::file_size(filepath) == filesize)
        return true;
    return false;
}

}

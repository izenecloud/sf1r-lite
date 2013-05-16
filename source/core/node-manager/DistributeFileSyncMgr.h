#ifndef SF1R_NODEMANAGER_DISTRIBUTE_FILESYNCMGR_H
#define SF1R_NODEMANAGER_DISTRIBUTE_FILESYNCMGR_H

#include "DistributeFileSyncRequest.h"
#include <string>
#include <util/singleton.h>
#include <3rdparty/msgpack/rpc/server.h>
#include <vector>
#include <boost/thread.hpp>
#include <boost/threadpool.hpp>

namespace sf1r
{

class FileSyncServer: public msgpack::rpc::server::base
{
public:
    FileSyncServer(const std::string& host, uint16_t port, uint32_t threadNum);
    ~FileSyncServer();
    const std::string& getHost() const
    {
        return host_;
    }
    inline uint16_t getPort() const
    {
        return port_;
    }

    void start();

    void join();

    // start + join
    void run();

    void stop();

public:
    virtual void dispatch(msgpack::rpc::request req);

private:
    std::string host_;
    uint16_t port_;
    uint32_t threadNum_;
    boost::threadpool::pool threadpool_;
};

class RpcServerConnection;

struct FileCheckData
{
    size_t file_size;
    time_t last_modify;
    std::string check_sum;
};

class DistributeFileSyncMgr
{
public:
    static DistributeFileSyncMgr* get()
    {
        return ::izenelib::util::Singleton<DistributeFileSyncMgr>::get();
    }

    DistributeFileSyncMgr();
    ~DistributeFileSyncMgr();
    void init();
    void stop();
    bool getNewestReqLog(bool from_primary_only, uint32_t start_from, std::vector<std::string>& saved_log);
    bool syncNewestSCDFileList(const std::string& colname);
    bool syncCollectionData(const std::vector<std::string>& colname_list);
    bool getFileFromOther(const std::string& filepath, bool force_overwrite = false);
    void notifyFinishReceive(const std::string& filepath);
    void sendFinishNotifyToReceiver(const std::string& ip, uint16_t port, const FinishReceiveRequest& req);
    bool pushFileToAllReplicas(const std::string& srcpath, const std::string& destpath, bool recrusive = false);

    void checkReplicasStatus(const std::vector<std::string>& colname_list, std::string& check_errinfo);
    void checkReplicasLogStatus(std::string& check_errinfo);
    void notifyReportStatusRsp(const ReportStatusRspData& rspdata);
    void sendReportStatusRsp(const std::string& ip, uint16_t port, const ReportStatusRsp& rsp);
    bool getCachedCheckSum(const std::string& filepath, std::string& ret_checksum);
    void updateCachedCheckSum(const std::string& filepath, const std::string& checksum);

private:
    void saveCachedCheckSum();
    void loadCachedCheckSum();
    bool getFileInfo(const std::string& ip, uint16_t port, GetFileData& fileinfo);
    bool getFileFromOther(const std::string& ip, uint16_t port, const std::string& filepath, uint64_t filesize, bool force_overwrite = false);
    RpcServerConnection* conn_mgr_;
    boost::shared_ptr<FileSyncServer> transfer_rpcserver_;
    boost::mutex mutex_;
    boost::condition_variable cond_;
    std::map<std::string, bool> wait_finish_notify_;
    boost::mutex status_report_mutex_;
    bool  reporting_;
    boost::condition_variable status_report_cond_;
    std::vector<ReportStatusRspData>  status_rsp_list_;
    std::set<std::string>  ignore_list_;
    std::map<std::string, FileCheckData> cached_checksum_;
};

}

#endif

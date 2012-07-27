#ifndef RPC_LOG_SERVER_H_
#define RPC_LOG_SERVER_H_

#include "LogServerStorage.h"
#include <log-manager/LogServerRequest.h>
#include <recommend-manager/item/MultiCollectionItemIdGenerator.h>

#include <3rdparty/msgpack/rpc/server.h>

#include <boost/scoped_ptr.hpp>

namespace sf1r
{
class LogServerWorkThread;
class RpcLogServer : public msgpack::rpc::server::base
{
public:
    RpcLogServer(const std::string& host, uint16_t port, uint32_t threadNum);

    ~RpcLogServer();

    bool init();

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

    /// Asynchronous update
    void updateUUID(const UUID2DocidList& uuid2DocidList);
    void AddOldUUID(OldUUIDData& reqdata);
    void AddOldDocId(OldDocIdData& reqdata);
    void DelOldDocId(const DelOldDocIdData& reqdata);
    void GetOldUUID(OldUUIDData& reqdata);
    void GetOldDocId(OldDocIdData& reqdata);

    void synchronize(const SynchronizeData& syncReqData);

    /// Will be called when update is actually performed
    void onUpdate(
        const LogServerStorage::uuid_t& uuid,
        const LogServerStorage::raw_docid_list_t& docidList,
        const std::string& aux);

    /// Add new scd doc
    void createScdDoc(const CreateScdDocRequestData& scdDoc);
    /// Delete scd doc
    void deleteScdDoc(const DeleteScdDocRequestData& delReq);

    /// Dispatch scd file to required server
    void dispatchScdFile(const GetScdFileRequestData& scdFileRequestData, GetScdFileResponseData& response);
    void writeScdDoc(std::ofstream& of, const std::string& doc, const uint128_t& docid);

private:
    void strIdToItemId_(msgpack::rpc::request& req);
    void itemIdToStrId_(msgpack::rpc::request& req);
    void getMaxItemId_(msgpack::rpc::request& req);

private:
    std::string host_;
    uint16_t port_;
    uint32_t threadNum_;

    boost::scoped_ptr<LogServerWorkThread> workerThread_;

    MultiCollectionItemIdGenerator itemIdGenerator_;
};

}

#endif /* RPC_LOG_SERVER_H_ */

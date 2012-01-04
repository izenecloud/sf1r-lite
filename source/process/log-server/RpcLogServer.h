#ifndef RPC_LOG_SERVER_H_
#define RPC_LOG_SERVER_H_

#include "LogServerStorage.h"

#include <log-manager/LogServerRequest.h>

#include <3rdparty/msgpack/rpc/server.h>

#include <boost/thread.hpp>

namespace sf1r
{

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

    /// Will be called when update is actually performed
    void onUpdate(
            const std::string& uuid,
            const std::vector<uint32_t>& docidList,
            const std::string& aux);

private:
    std::string host_;
    uint16_t port_;
    uint32_t threadNum_;

    LogServerStorage::DrumPtr drum_;
    LogServerStorage::KVDBPtr docidDB_;

    boost::mutex mutex_;
};

}

#endif /* RPC_LOG_SERVER_H_ */

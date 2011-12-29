#ifndef RPC_LOG_SERVER_H_
#define RPC_LOG_SERVER_H_

#include "LogServerStorage.h"

#include <3rdparty/msgpack/rpc/server.h>

namespace sf1r
{

class RpcLogServer : public msgpack::rpc::server::base
{
public:
    RpcLogServer(const std::string& host, uint16_t port, uint32_t threadNum);

    ~RpcLogServer();

    void setDrum(const LogServerStorage::DrumPtr& drum)
    {
        drum_ = drum;
    }

    uint16_t getPort() const
    {
        return port_;
    }

    void start();

    void join()
    {
        instance.join();
    }

    void run()
    {
        start();
        join();
    }

    void stop()
    {
        instance.end();
        instance.join();
    }

public:
    virtual void dispatch(msgpack::rpc::request req);

private:
    std::string host_;
    uint16_t port_;
    uint32_t threadNum_;

    LogServerStorage::DrumPtr drum_;
};

}

#endif /* RPC_LOG_SERVER_H_ */

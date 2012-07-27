#ifndef RPC_COLOR_SERVER_H_
#define RPC_COLOR_SERVER_H_

#include <3rdparty/msgpack/rpc/server.h>

namespace sf1r
{
class RpcImageServer : public msgpack::rpc::server::base
{
public:
    RpcImageServer(const std::string& host, uint16_t port, uint32_t threadNum);

    ~RpcImageServer();

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
};

}

#endif /* RPC_COLOR_SERVER_H_ */

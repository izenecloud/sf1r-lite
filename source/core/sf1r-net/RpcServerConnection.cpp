#include "RpcServerConnection.h"

namespace sf1r
{

RpcServerConnection::RpcServerConnection()
    : need_flush_(false)
{
}

RpcServerConnection::~RpcServerConnection()
{
    session_pool_.reset();
}

bool RpcServerConnection::init(const RpcServerConnectionConfig& config)
{
    if (session_pool_)
    {
        std::cerr << "RpcServerConnection has already inited!! " << std::endl;
        return false;
    }
    config_ = config;

    try
    {
        session_pool_.reset(new msgpack::rpc::session_pool);
        session_pool_->start(config_.rpcThreadNum);
    }
    catch (const std::exception& e)
    {
        std::cerr << "RpcServerConnection: " << e.what() << std::endl;
        return false;
    }

    return true;
}

bool RpcServerConnection::testServer()
{
    return testServer(config_.host, config_.rpcPort);
}

void RpcServerConnection::flushRequests()
{
    if (need_flush_)
    {
        flushRequests(config_.host, config_.rpcPort);
        need_flush_ = false;
    }
}

bool RpcServerConnection::testServer(const std::string& ip, uint16_t port)
{
    try
    {
        msgpack::rpc::session session = session_pool_->get_session(ip, port);

        session.set_timeout(10);

        bool ret = session.call(
                        "test",
                        true
                        ).get<bool>();
        return ret;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Rpc Server (" << ip << ":" << port << "): "
                  << e.what() << std::endl;
        return false;
    }
}

void RpcServerConnection::flushRequests(const std::string& ip, uint16_t port)
{
    msgpack::rpc::session session = session_pool_->get_session(ip, port);
    session.get_loop()->flush();
}

}

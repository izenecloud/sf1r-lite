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
    try
    {
        msgpack::rpc::session session = session_pool_->get_session(config_.host, config_.rpcPort);

        bool ret = session.call(
                        "test",
                        true
                        ).get<bool>();
        return ret;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Rpc Server (" << config_.host << ":" << config_.rpcPort << "): "
                  << e.what() << std::endl;
        return false;
    }
}

void RpcServerConnection::flushRequests()
{
    if (need_flush_)
    {
        msgpack::rpc::session session = session_pool_->get_session(config_.host, config_.rpcPort);
        session.get_loop()->flush();
        need_flush_ = false;
    }
}

}

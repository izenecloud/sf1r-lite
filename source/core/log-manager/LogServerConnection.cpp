#include "LogServerConnection.h"

namespace sf1r
{

LogServerConnection::LogServerConnection()
    : need_flush_(false)
{
}

LogServerConnection::~LogServerConnection()
{
}

bool LogServerConnection::init(const LogServerConnectionConfig& config)
{
    config_ = config;

    try
    {
        session_pool_.reset(new msgpack::rpc::session_pool);
        session_pool_->start(config_.rpcThreadNum);
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return false;
    }

    return true;
}

void LogServerConnection::flushRequests()
{
    if (need_flush_)
    {
        msgpack::rpc::session session = session_pool_->get_session(config_.host, config_.rpcPort);
        session.get_loop()->flush();
        need_flush_ = false;
    }
}

}

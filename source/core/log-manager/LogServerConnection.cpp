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
        std::cerr << "LogServerConnection: " << e.what() << std::endl;
        return false;
    }

    return true;
}

bool LogServerConnection::testServer()
{
    try
    {
        msgpack::rpc::session session = session_pool_->get_session(config_.host, config_.rpcPort);

        bool ret = session.call(
                        LogServerRequest::method_names[LogServerRequest::METHOD_TEST],
                        true
                        ).get<bool>();
        return ret;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Log Server (" << config_.host << ":" << config_.rpcPort << "): "
                  << e.what() << std::endl;
        return false;
    }
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

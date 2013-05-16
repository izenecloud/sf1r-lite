#include "LogAnalysisConnection.h"

namespace sf1r
{

LogAnalysisConnection::LogAnalysisConnection()
    : need_flush_(false)
{
}

LogAnalysisConnection::~LogAnalysisConnection()
{
    session_pool_.reset();
}

bool LogAnalysisConnection::init(const LogServerConnectionConfig& config)
{
    config_ = config;

    try
    {
        session_pool_.reset(new msgpack::rpc::session_pool);
        session_pool_->start(config_.rpcThreadNum);
    }
    catch (const std::exception& e)
    {
        std::cerr << "LogAnalysisConnection: " << e.what() << std::endl;
        return false;
    }

    return true;
}

bool LogAnalysisConnection::testServer()
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

void LogAnalysisConnection::flushRequests()
{
    if (need_flush_)
    {
        msgpack::rpc::session session = session_pool_->get_session(config_.host, config_.rpcPort);
        session.get_loop()->flush();
        need_flush_ = false;
    }
}

}

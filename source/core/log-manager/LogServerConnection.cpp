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

bool LogServerConnection::init(const std::string& host, uint16_t port)
{
    host_ = host;
    port_ = port;

    try
    {
        client_.reset(new msgpack::rpc::client(host_, port_));
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
        client_->get_loop()->flush();
        need_flush_ = false;
    }
}

}

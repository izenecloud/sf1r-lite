#include "LogServerConnection.h"

namespace sf1r
{

LogServerConnection::LogServerConnection()
    : bInited_(false)
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
        msgpack::rpc::client cli(host_, port_);
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return false;
    }

    return true;
}

}

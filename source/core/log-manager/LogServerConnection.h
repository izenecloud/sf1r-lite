#ifndef _LOG_SERVER_CONNECTION_H_
#define _LOG_SERVER_CONNECTION_H_

#include "LogServerRequest.h"
#include "LogManagerSingleton.h"

#include <3rdparty/msgpack/rpc/client.h>

namespace sf1r
{

class LogServerConnection : public LogManagerSingleton<LogServerConnection>
{
public:
    LogServerConnection()
    : bInited_(false)
    {
    }

    void init(const std::string& host, uint16_t port)
    {
        host_ = host;
        port_ = port;
    }

    template <typename Request>
    void asynRequest(LogServerRequest::method_t method, Request req);

private:
    bool bInited_;
    std::string host_;
    uint16_t port_;
};

template <typename Request>
void LogServerConnection::asynRequest(LogServerRequest::method_t method, Request req)
{
    msgpack::rpc::client cli(host_, port_);
    cli.call(method, req);
}

}

#endif /* _LOG_SERVER_CONNECTION_H_ */

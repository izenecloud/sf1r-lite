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

    ~LogServerConnection()
    {
    }

    bool init(const std::string& host, uint16_t port)
    {
        host_ = host;
        port_ = port;

        try
        {
            msgpack::rpc::client cli(host_, port_);
        }
        catch(std::exception& e)
        {
            std::cerr<<e.what()<<std::endl;
            return false;
        }

        return true;
    }

    template <typename RequestDataT>
    void asynRequest(LogServerRequest::method_t method, RequestDataT& reqData);

    template <typename RequestT>
    void asynRequest(RequestT& req);

private:
    bool bInited_;
    std::string host_;
    uint16_t port_;
};

template <typename RequestDataT>
void LogServerConnection::asynRequest(LogServerRequest::method_t method, RequestDataT& reqData)
{
    msgpack::rpc::client cli(host_, port_);

    //cli.call(method, reqData);
    cli.notify(method, reqData);
    cli.get_loop()->flush();
}

template <typename RequestT>
void LogServerConnection::asynRequest(RequestT& req)
{
    asynRequest(req.method_, req.param_);
}

}

#endif /* _LOG_SERVER_CONNECTION_H_ */

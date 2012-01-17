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
    LogServerConnection();

    ~LogServerConnection();

    bool init(const std::string& host, uint16_t port);

    template <class RequestDataT>
    void asynRequest(const LogServerRequest::method_t& method, const RequestDataT& reqData);

    template <class RequestT>
    void asynRequest(const RequestT& req);

    template <class RequestDataT, class ResponseDataT>
    void syncRequest(const LogServerRequest::method_t& method, const RequestDataT& reqData, ResponseDataT& respData);

    template <class RequestT, class ResponseDataT>
    void syncRequest(const RequestT& req, ResponseDataT& respData);

    void flushRequests();

private:
    bool need_flush_;
    std::string host_;
    uint16_t port_;
    boost::shared_ptr<msgpack::rpc::client> client_;
};

template <class RequestDataT>
void LogServerConnection::asynRequest(const LogServerRequest::method_t& method, const RequestDataT& reqData)
{
    static unsigned int count = 0;
    client_->notify(method, reqData);
    need_flush_ = true;
    if (++count == 1000)
    {
        flushRequests();
        count = 0;
    }
}

template <class RequestT>
void LogServerConnection::asynRequest(const RequestT& req)
{
    asynRequest(req.method_names[req.method_], req.param_);
}

template <class RequestDataT, class ResponseDataT>
void LogServerConnection::syncRequest(const LogServerRequest::method_t& method, const RequestDataT& reqData, ResponseDataT& respData)
{
    flushRequests();
    respData = client_->call(method, reqData).get<ResponseDataT>();
}

template <class RequestT, class ResponseDataT>
void LogServerConnection::syncRequest(const RequestT& req, ResponseDataT& respData)
{
    syncRequest(req.method_names[req.method_], req.param_, respData);
}

}

#endif /* _LOG_SERVER_CONNECTION_H_ */

#ifndef _LOG_SERVER_CONNECTION_H_
#define _LOG_SERVER_CONNECTION_H_

#include "LogServerRequest.h"
#include "LogManagerSingleton.h"

#include <3rdparty/msgpack/rpc/client.h>
#include <3rdparty/msgpack/rpc/session_pool.h>

#include <boost/scoped_ptr.hpp>

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
    boost::scoped_ptr<msgpack::rpc::session_pool> session_pool_;
};

template <class RequestDataT>
void LogServerConnection::asynRequest(const LogServerRequest::method_t& method, const RequestDataT& reqData)
{
    static unsigned int count = 0;
    msgpack::rpc::session session = session_pool_->get_session(host_, port_);
    session.notify(method, reqData);
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
    msgpack::rpc::session session = session_pool_->get_session(host_, port_);
    respData = session.call(method, reqData).get<ResponseDataT>();
}

template <class RequestT, class ResponseDataT>
void LogServerConnection::syncRequest(const RequestT& req, ResponseDataT& respData)
{
    syncRequest(req.method_names[req.method_], req.param_, respData);
}

}

#endif /* _LOG_SERVER_CONNECTION_H_ */

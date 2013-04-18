#ifndef _RPC_SERVER_CONNECTION_H_
#define _RPC_SERVER_CONNECTION_H_

#include "RpcServerConnectionConfig.h"
#include <3rdparty/msgpack/rpc/client.h>
#include <3rdparty/msgpack/rpc/session_pool.h>
#include <boost/scoped_ptr.hpp>
#include <util/singleton.h>

namespace sf1r
{

class RpcServerConnection 
{
public:
    RpcServerConnection();
    ~RpcServerConnection();

    static RpcServerConnection& instance()
    {
        return *(izenelib::util::Singleton<RpcServerConnection>::get());
    }

    bool init(const RpcServerConnectionConfig& config);

    // the rpc server should return a response to the method "test" 
    bool testServer();

    template <class Method_T, class RequestDataT>
    void asynRequest(const Method_T& method, const RequestDataT& reqData);

    template <class RequestT>
    void asynRequest(const RequestT& req);

    template <class Method_T, class RequestDataT, class ResponseDataT>
    void syncRequest(const Method_T& method, const RequestDataT& reqData, ResponseDataT& respData);

    template <class RequestT, class ResponseDataT>
    void syncRequest(const RequestT& req, ResponseDataT& respData);

    void flushRequests();
    //
    // the rpc server should return a response to the method "test" 
    bool testServer(const std::string& ip, uint16_t port);

    template <class Method_T, class RequestDataT>
    void asynRequest(const std::string& ip, uint16_t port, const Method_T& method, const RequestDataT& reqData);

    template <class RequestT>
    void asynRequest(const std::string& ip, uint16_t port, const RequestT& req);

    template <class Method_T, class RequestDataT, class ResponseDataT>
    void syncRequest(const std::string& ip, uint16_t port, const Method_T& method, const RequestDataT& reqData, ResponseDataT& respData);

    template <class RequestT, class ResponseDataT>
    void syncRequest(const std::string& ip, uint16_t port, const RequestT& req, ResponseDataT& respData);

    void flushRequests(const std::string& ip, uint16_t port);

private:
    bool need_flush_;
    RpcServerConnectionConfig config_;
    boost::scoped_ptr<msgpack::rpc::session_pool> session_pool_;
};

template <class Method_T, class RequestDataT>
void RpcServerConnection::asynRequest(const Method_T& method, const RequestDataT& reqData)
{
    static unsigned int count = 0;
    msgpack::rpc::session session = session_pool_->get_session(config_.host, config_.rpcPort);
    session.notify(method, reqData);
    need_flush_ = true;
    if (++count == 10000)
    {
        flushRequests();
        count = 0;
    }
}

template <class RequestT>
void RpcServerConnection::asynRequest(const RequestT& req)
{
    asynRequest(req.method_names[req.method_], req.param_);
}

template <class Method_T, class RequestDataT, class ResponseDataT>
void RpcServerConnection::syncRequest(const Method_T& method, const RequestDataT& reqData, ResponseDataT& respData)
{
    flushRequests();
    msgpack::rpc::session session = session_pool_->get_session(config_.host, config_.rpcPort);
    session.set_timeout(10);
    respData = session.call(method, reqData).template get<ResponseDataT>();
}

template <class RequestT, class ResponseDataT>
void RpcServerConnection::syncRequest(const RequestT& req, ResponseDataT& respData)
{
    syncRequest(req.method_names[req.method_], req.param_, respData);
}

template <class Method_T, class RequestDataT>
void RpcServerConnection::asynRequest(const std::string& ip, uint16_t port, const Method_T& method, const RequestDataT& reqData)
{
    static unsigned int count = 0;
    msgpack::rpc::session session = session_pool_->get_session(ip, port);
    session.notify(method, reqData);
    if (++count == 10000)
    {
        flushRequests(ip, port);
        count = 0;
    }
}

template <class RequestT>
void RpcServerConnection::asynRequest(const std::string& ip, uint16_t port, const RequestT& req)
{
    asynRequest(ip, port, req.method_names[req.method_], req.param_);
}

template <class Method_T, class RequestDataT, class ResponseDataT>
void RpcServerConnection::syncRequest(const std::string& ip, uint16_t port, const Method_T& method, const RequestDataT& reqData, ResponseDataT& respData)
{
    flushRequests(ip, port);
    msgpack::rpc::session session = session_pool_->get_session(ip, port);
    session.set_timeout(10);
    respData = session.call(method, reqData).template get<ResponseDataT>();
}

template <class RequestT, class ResponseDataT>
void RpcServerConnection::syncRequest(const std::string& ip, uint16_t port, const RequestT& req, ResponseDataT& respData)
{
    syncRequest(ip, port, req.method_names[req.method_], req.param_, respData);
}


}

#endif /* _RPC_SERVER_CONNECTION_H_ */

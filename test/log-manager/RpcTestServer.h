///
/// @file RpcTestServer.h
/// @brief a simple RPC server used to test LogServerConnection
/// @author Jun Jiang
/// @date Created 2012-02-08
///

#ifndef RPC_TEST_SERVER_H
#define RPC_TEST_SERVER_H

#include <3rdparty/msgpack/rpc/server.h>

#include <boost/function.hpp>
#include <map>
#include <string>

namespace sf1r
{

class RpcTestServer : public msgpack::rpc::server::base
{
public:
    ~RpcTestServer();

    void stop();

    virtual void dispatch(msgpack::rpc::request req);

    /**
     * Register a method to be called in @c dispatch().
     * after @p method is registered, in @c dispatch(), it would find the method
     * with @p name, and call it with the signature: void MethodT(msgpack::rpc::request)
     */
    template <typename MethodT>
    void registerMethod(const std::string& name, MethodT method)
    {
        methodMap_[name] = method;
    }

private:
    typedef boost::function<void(msgpack::rpc::request)> MethodType;
    typedef std::map<std::string, MethodType> MethodMap; /// name -> method
    MethodMap methodMap_;
};

} // namespace sf1r

#endif //RPC_TEST_SERVER_H

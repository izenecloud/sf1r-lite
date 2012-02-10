#include "RpcTestServer.h"

namespace sf1r
{

RpcTestServer::~RpcTestServer()
{
    stop();
}

void RpcTestServer::stop()
{
    if (instance.is_running())
    {
        instance.end();
        instance.join();
    }
}

void RpcTestServer::dispatch(msgpack::rpc::request req)
{
    try
    {
        std::string name;
        req.method().convert(&name);

        MethodMap::const_iterator it = methodMap_.find(name);
        if (it != methodMap_.end())
        {
            it->second(req);
        }
        else
        {
            req.error(msgpack::rpc::NO_METHOD_ERROR);
        }
    }
    catch (const msgpack::type_error& e)
    {
        req.error(msgpack::rpc::ARGUMENT_ERROR);
    }
    catch (const std::exception& e)
    {
        req.error(std::string(e.what()));
    }
}

} // namespace sf1r

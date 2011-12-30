#include "RpcLogServer.h"

#include <log-manager/LogServerRequest.h>

namespace sf1r
{

RpcLogServer::RpcLogServer(const std::string& host, uint16_t port, uint32_t threadNum)
    : host_(host)
    , port_(port)
    , threadNum_(threadNum)
{
}

RpcLogServer::~RpcLogServer()
{
    stop();
}

void RpcLogServer::start()
{
    instance.listen(host_, port_);
    instance.start(threadNum_);
}

void RpcLogServer::join()
{
    instance.join();
}

void RpcLogServer::run()
{
    start();
    join();
}

void RpcLogServer::stop()
{
    instance.end();
    instance.join();
}

void RpcLogServer::dispatch(msgpack::rpc::request req)
{
    try
    {
        std::string method;
        req.method().convert(&method);

        if (method == LogServerRequest::METHOD_UPDATE_UUID)
        {
            msgpack::type::tuple<UUID2DocIdList> params;
            req.params().convert(&params);
            UUID2DocIdList uuid2DocIdList = params.get<0>();

            // TODO
            std::cout << uuid2DocIdList.uuid_ << std::endl;
            //drum_->CheckUpdate(uuid2DocIdList.uuid_, uuid2DocIdList.docIdList_);
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

}

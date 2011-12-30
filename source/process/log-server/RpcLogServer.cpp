#include "RpcLogServer.h"

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
            msgpack::type::tuple<UUID2DocidList> params;
            req.params().convert(&params);
            UUID2DocidList uuid2DocidList = params.get<0>();

            onUpdateUUID(uuid2DocidList);
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

void RpcLogServer::onUpdateUUID(const UUID2DocidList& uuid2DocidList)
{
    // xxx, drum is not thread safe
    std::cout << uuid2DocidList.toString() << std::endl;
    drum_->Update(uuid2DocidList.uuid_, uuid2DocidList.docidList_);

    UUID2DocidList::DocidListType::const_iterator it;
    for (it = uuid2DocidList.docidList_.begin(); it != uuid2DocidList.docidList_.end(); it++)
    {
        std::cout<<*it<<" -> "<<uuid2DocidList.uuid_<<std::endl;
        docidDB_->update(*it, uuid2DocidList.uuid_);
    }
}

}

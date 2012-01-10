#include "RpcLogServer.h"

#include <common/Utilities.h>

#include <boost/bind.hpp>

//#define LOG_SERVER_DEBUG 1

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

bool RpcLogServer::init()
{
    drum_ = LogServerStorage::get()->getDrum();
    docidDB_ = LogServerStorage::get()->getDocidDB();

    LogServerStorage::get()->getDrumDispatcher().registerOp(
            LogServerStorage::DrumDispatcherImpl::UPDATE,
            boost::bind(&RpcLogServer::onUpdate, this, _1, _2, _3));

    return true;
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

        if (method == LogServerRequest::method_names[LogServerRequest::METHOD_UPDATE_UUID])
        {
            msgpack::type::tuple<UUID2DocidList> params;
            req.params().convert(&params);
            const UUID2DocidList& uuid2DocidList = params.get<0>();

            updateUUID(uuid2DocidList);
        }
        else if (method == LogServerRequest::method_names[LogServerRequest::METHOD_SYNCHRONIZE])
        {
            synchronize();
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

void RpcLogServer::synchronize()
{
    boost::lock_guard<boost::mutex> lock(mutex_);
    drum_->Synchronize();
    docidDB_->flush();
}

void RpcLogServer::updateUUID(const UUID2DocidList& uuid2DocidList)
{
    // drum is not thread safe
    boost::lock_guard<boost::mutex> lock(mutex_);

#ifdef LOG_SERVER_DEBUG
    std::cout << uuid2DocidList.toString() << std::endl; //xxx
#endif
    drum_->Update(uuid2DocidList.uuid_, uuid2DocidList.docidList_);
}

void RpcLogServer::onUpdate(
        const LogServerStorage::drum_key_t& uuid,
        const LogServerStorage::drum_value_t& docidList,
        const LogServerStorage::drum_aux_t& aux)
{
#ifdef LOG_SERVER_DEBUG
    std::cout << "RpcLogServer::onUpDate " << std::endl; //xxx
#endif

    for (LogServerStorage::drum_value_t::const_iterator it = docidList.begin();
            it != docidList.end(); ++it)
    {
#ifdef LOG_SERVER_DEBUG
        std::cout << *it << " -> " << Utilities::uint128ToUuid(uuid) << std::endl;
#endif
        docidDB_->update(*it, uuid);
    }
}

}

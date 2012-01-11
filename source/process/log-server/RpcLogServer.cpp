#include "RpcLogServer.h"

#include <common/Utilities.h>

#include <boost/bind.hpp>

//#define LOG_SERVER_DEBUG
//#define DOCID_DB_DEBUG

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

    workerThread_.reset(new LogServerWorkThread(drum_, docidDB_));

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
            msgpack::type::tuple<SynchronizeData> params;
            req.params().convert(&params);
            const SynchronizeData& syncReqData = params.get<0>();

            synchronize(syncReqData);
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

void RpcLogServer::updateUUID(const UUID2DocidList& uuid2DocidList)
{
#ifdef LOG_SERVER_DEBUG
    std::cout << "updateUUID: " << uuid2DocidList.toString() << std::endl;
#endif
    workerThread_->putUuidRequestData(uuid2DocidList);
}

void RpcLogServer::synchronize(const SynchronizeData& syncReqData)
{
    std::cout << "Received request for synchronizing." << std::endl;
    workerThread_->putSyncRequestData(syncReqData);
}

void RpcLogServer::onUpdate(
        const LogServerStorage::drum_key_t& uuid,
        const LogServerStorage::drum_value_t& docidList,
        const LogServerStorage::drum_aux_t& aux)
{
#ifdef LOG_SERVER_DEBUG
    std::cout << "RpcLogServer::onUpDate " << std::endl;
#endif

#ifdef DOCID_DB_DEBUG
    static int cnt;
#endif

    boost::lock_guard<boost::mutex> lock(LogServerStorage::get()->docidDBMutex());
    for (LogServerStorage::drum_value_t::const_iterator it = docidList.begin();
            it != docidList.end(); ++it)
    {
        docidDB_->update(*it, uuid);

#ifdef LOG_SERVER_DEBUG
        std::cout << *it << " -> " << Utilities::uint128ToUuid(uuid) << std::endl;
#endif

#ifdef DOCID_DB_DEBUG
        if ((++cnt % 1000) == 0)
        {
            std::cout << "updated to docid DB: " << cnt << std::endl;
        }
#endif
    }
}

}

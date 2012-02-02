#include "RpcLogServer.h"

#include <common/Utilities.h>
#include <common/ScdParser.h>
#include <common/ScdWriter.h>

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
    std::cout << "~RpcLogServer()" << std::endl;
    stop();
}

bool RpcLogServer::init()
{
    LogServerStorage::get()->uuidDrumDispatcher().registerOp(
            LogServerStorage::UuidDrumDispatcherType::UPDATE,
            boost::bind(&RpcLogServer::onUpdate, this, _1, _2, _3));

    workerThread_.reset(new LogServerWorkThread());

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
    workerThread_->stop();
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
        else if (method == LogServerRequest::method_names[LogServerRequest::METHOD_GET_UUID])
        {
            msgpack::type::tuple<Docid2UUID> params;
            req.params().convert(&params);
            Docid2UUID& docid2UUID = params.get<0>();

            LogServerStorage::get()->docidDrum()->GetValue(docid2UUID.docid_, docid2UUID.uuid_);
            req.result(docid2UUID);
        }
        else if (method == LogServerRequest::method_names[LogServerRequest::METHOD_GET_DOCID_LIST])
        {
            msgpack::type::tuple<UUID2DocidList> params;
            req.params().convert(&params);
            UUID2DocidList& uuid2DocidList = params.get<0>();

            LogServerStorage::get()->uuidDrum()->GetValue(uuid2DocidList.uuid_, uuid2DocidList.docidList_);
            req.result(uuid2DocidList);
        }
        else if (method == LogServerRequest::method_names[LogServerRequest::METHOD_CREATE_SCD_DOC])
        {
            msgpack::type::tuple<CreateScdDocRequestData> params;
            req.params().convert(&params);
            const CreateScdDocRequestData& scdDoc = params.get<0>();

            createScdDoc(scdDoc);
        }
        else if (method == LogServerRequest::method_names[LogServerRequest::METHOD_GET_SCD_FILE])
        {
            msgpack::type::tuple<GetScdFileRequestData> params;
            req.params().convert(&params);
            const GetScdFileRequestData& scdFileRequestData = params.get<0>();

            dispatchScdFile(scdFileRequestData);
            //req.result();
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
    std::cout << "UUID -> DOCIDs: " << uuid2DocidList.toString() << std::endl;
#endif
    workerThread_->putUuidRequestData(uuid2DocidList);
}

void RpcLogServer::synchronize(const SynchronizeData& syncReqData)
{
    std::cout << "Received rpc request for synchronizing." << std::endl;
    workerThread_->putSyncRequestData(syncReqData);
}

void RpcLogServer::onUpdate(
        const LogServerStorage::uuid_t& uuid,
        const LogServerStorage::raw_docid_list_t& docidList,
        const std::string& aux)
{
#ifdef DOCID_DB_DEBUG
    static int cnt;
#endif

    boost::lock_guard<boost::mutex> lock(LogServerStorage::get()->docidDrumMutex());
    for (LogServerStorage::raw_docid_list_t::const_iterator it = docidList.begin();
            it != docidList.end(); ++it)
    {
        LogServerStorage::get()->docidDrum()->Update(*it, uuid);

#ifdef LOG_SERVER_DEBUG
        std::cout << "DOCID -> UUID: "<< *it << " -> " << Utilities::uint128ToUuid(uuid) << std::endl;
#endif

#ifdef DOCID_DB_DEBUG
        if ((++cnt % 10000) == 0)
        {
            std::cout << "updated to docid DB: " << cnt << std::endl;
        }
#endif
    }
}

void RpcLogServer::createScdDoc(const CreateScdDocRequestData& scdDoc)
{
    boost::lock_guard<boost::mutex> lock(LogServerStorage::get()->scdDbMutex());
    LogServerStorage::get()->scdDb()->update(scdDoc.uuid_, scdDoc.content_);

#ifdef LOG_SERVER_DEBUG
    std::cout << "--> Create SCD Doc: " << Utilities::uint128ToUuid(scdDoc.uuid_) << std::endl;
    std::cout << scdDoc.content_ << std:: endl;
#endif
}

bool RpcLogServer::dispatchScdFile(const GetScdFileRequestData& scdFileRequestData)
{
    boost::lock_guard<boost::mutex> lock(LogServerStorage::get()->scdDbMutex());
    LogServerStorage::ScdDbPtr& scdDb = LogServerStorage::get()->scdDb();

    boost::lock_guard<boost::mutex> lockUuidDrum(LogServerStorage::get()->uuidDrumMutex());
    boost::lock_guard<boost::mutex> lockDocidDrum(LogServerStorage::get()->docidDrumMutex());

    // create new scd file
    std::string filename = LogServerCfg::get()->getStorageBaseDir() + "/" + ScdWriter::GenSCDFileName(INSERT_SCD);
    std::cout << filename << std::endl << std::endl;

    std::ofstream of;
    of.open(filename.c_str());
    if (!of.good())
    {
        std::cerr << "Failed to create SCD file: " << filename << std::endl;
        return false;
    }

    // get all scd docs from DB
    uint128_t uuid;
    std::string content;

#ifdef USE_TC_HASH
    LogServerStorage::ScdDbType::SDBCursor locn = scdDb->get_first_locn();
    while ( scdDb->get(locn, uuid, content) )
    {
        writeScdDoc(of, content, uuid);
        scdDb->seq(locn);
    }
#else
    if (scdDb->iterInit())
    {
        while (scdDb->iterNext(uuid, content))
        {
            writeScdDoc(of, content, uuid);
        }
    }
    else
    {
        of.close();
        return false;
    }
#endif

    of.close();

    // transmit SCD file to required server
    std::stringstream command;
    command << "rsync " << "-vaz " << filename << " localhost::home/zhongxia/"; // Fixme
    std::cout << command.str() << std::endl;
    if (std::system(command.str().c_str()) == 0)
        return true;
    else
        return false;
}

void RpcLogServer::writeScdDoc(std::ofstream& of, const std::string& doc, const uint128_t& uuid)
{
    std::string oldUuidStr = Utilities::uint128ToUuid(uuid);

    UUID2DocidList::DocidListType docidList;
    LogServerStorage::get()->uuidDrum()->GetValue(uuid, docidList);

    if (docidList.empty())
    {
        // no need to update uuid (docid)
        of << doc;

#ifdef LOG_SERVER_DEBUG
        std::cout << "--> writeScdDoc : " << oldUuidStr << std::endl;
        std::cout << doc << std:: endl;
#endif
    }
    else
    {
        uint128_t newUuid;
        std::string newUuidStr;

        std::set<uint128_t> uniqueUuidSet;
        pair<std::set<uint128_t>::iterator, bool> ret;

        UUID2DocidList::DocidListType::iterator it;
        for (it = docidList.begin(); it != docidList.end(); ++it)
        {
            std::string newDoc = doc;
            if (LogServerStorage::get()->docidDrum()->GetValue(*it, newUuid))
            {
                // update uuid (docid)
                newUuidStr = Utilities::uint128ToUuid(newUuid);
                boost::replace_all(newDoc, oldUuidStr, newUuidStr);
            }

            // avoid duplicate
            ret = uniqueUuidSet.insert(newUuid);
            if (ret.second == false)
            {
                continue;
            }

            // write scd file
            of << doc;

#ifdef LOG_SERVER_DEBUG
            std::cout << "--> writeScdDoc : " << oldUuidStr << " -> " << newUuidStr << std::endl;
            std::cout << newDoc << std:: endl;
#endif
        }
    }
}

}

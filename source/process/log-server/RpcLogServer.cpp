#include "RpcLogServer.h"

#include <common/Utilities.h>
#include <common/ScdParser.h>
#include <common/ScdWriter.h>

#include <boost/bind.hpp>
#include <boost/filesystem.hpp>

//#define LOG_SERVER_DEBUG
//#define DOCID_DB_DEBUG

namespace sf1r
{

const static std::string UUID_PROPERTY = "<UUID>";

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

    boost::filesystem::path dir(LogServerCfg::get()->getStorageBaseDir());
    dir /= "itemid";
    itemIdGenerator_.init(dir.string());

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
        else if (method == LogServerRequest::method_names[LogServerRequest::METHOD_DELETE_SCD_DOC])
        {
            msgpack::type::tuple<DeleteScdDocRequestData> params;
            req.params().convert(&params);
            const DeleteScdDocRequestData& delReq = params.get<0>();

            deleteScdDoc(delReq);
        }
        else if (method == LogServerRequest::method_names[LogServerRequest::METHOD_GET_SCD_FILE])
        {
            msgpack::type::tuple<GetScdFileRequestData> params;
            req.params().convert(&params);
            const GetScdFileRequestData& scdFileRequestData = params.get<0>();

            GetScdFileResponseData response;
            dispatchScdFile(scdFileRequestData, response);
            req.result(response);
        }
        else if (method == LogServerRequest::method_names[LogServerRequest::METHOD_STRID_TO_ITEMID])
        {
            strIdToItemId_(req);
        }
        else if (method == LogServerRequest::method_names[LogServerRequest::METHOD_ITEMID_TO_STRID])
        {
            itemIdToStrId_(req);
        }
        else if (method == LogServerRequest::method_names[LogServerRequest::METHOD_GET_MAX_ITEMID])
        {
            getMaxItemId_(req);
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
    const std::string& collection = scdDoc.collection_;
    if (collection.empty())
    {
        std::cerr << "CreateScdDocRequest error: missing collection parameter." << std::endl;
        return;
    }

#ifdef LOG_SERVER_DEBUG
    std::cout << "\n[Received doc for collection: " << scdDoc.collection_ << "]" << std::endl;
    std::cout << scdDoc.content_;
#endif

    if (LogServerStorage::get()->checkScdDb(collection))
    {
        boost::shared_ptr<LogServerStorage::ScdStorage>& scdStorage = LogServerStorage::get()->scdStorage(collection);

        boost::lock_guard<boost::mutex> lock(scdStorage->mutex_);
        if (scdStorage->isReIndexed_)
        {
#ifdef LOG_SERVER_DEBUG
            std::cout << "[It comes after re-index]" << std::endl;
#endif
            scdStorage->scdFile_ << scdDoc.content_;
            scdStorage->scdFile_.flush();
        }
        else
        {
            scdStorage->scdDb_->update(scdDoc.docid_, scdDoc.content_);
            scdStorage->scdDb_->flush();
        }
    }
    else
    {
        std::cerr << "Failed to createScdDoc for collection: " << collection << "" << std::endl;
        return;
    }
}

void RpcLogServer::deleteScdDoc(const DeleteScdDocRequestData& delReq)
{
    const std::string& collection = delReq.collection_;
    if (collection.empty())
    {
        std::cerr << "DeleteScdDocRequestData error: missing collection parameter." << std::endl;
        return;
    }

    if (LogServerStorage::get()->checkScdDb(collection))
    {
        boost::shared_ptr<LogServerStorage::ScdStorage>& scdStorage =
                LogServerStorage::get()->scdStorage(collection);

        boost::lock_guard<boost::mutex> lock(scdStorage->mutex_);
        scdStorage->scdDb_->del(delReq.docid_);
    }
}

void RpcLogServer::dispatchScdFile(const GetScdFileRequestData& scdFileRequestData, GetScdFileResponseData& response)
{
    std::cout << "\n[Fetching SCD for \""<< scdFileRequestData.collection_
              << "\" by "
              << scdFileRequestData.username_
              << "@"
              << scdFileRequestData.host_
              << "]" << std::endl;

    const std::string& collection = scdFileRequestData.collection_;
    if (collection.empty())
    {
        response.success_ = false;
        response.error_ = "LogServer: GetScdFileRequest missing collection parameter.";
        std::cerr << response.error_ << std::endl;
        return;
    }

    if (!LogServerStorage::get()->checkScdDb(collection))
    {
        response.success_ = false;
        response.error_ = "LogServer: no SCD DB for " + collection;
        std::cerr << response.error_ << std::endl;
        return;
    }

    boost::shared_ptr<LogServerStorage::ScdStorage>& scdStorage = LogServerStorage::get()->scdStorage(collection);
    boost::lock_guard<boost::mutex> lock(scdStorage->mutex_);
    LogServerStorage::ScdDbPtr& scdDb = scdStorage->scdDb_;

    boost::lock_guard<boost::mutex> lockUuidDrum(LogServerStorage::get()->uuidDrumMutex());
    boost::lock_guard<boost::mutex> lockDocidDrum(LogServerStorage::get()->docidDrumMutex());

    // create new scd file
    std::string scdFileName = ScdWriter::GenSCDFileName(INSERT_SCD);
    std::string filename = LogServerCfg::get()->getStorageBaseDir() + "/" + scdFileName;
    //std::cout << filename << std::endl << std::endl;

    std::ofstream of;
    of.open(filename.c_str());
    if (!of.good())
    {
        std::cerr << "Failed to create SCD file: " << filename << std::endl;
        response.success_ = false;
        response.error_ = "LogServer: failed to create SCD file.";
        return;
    }

    // get all scd docs from DB
    uint128_t docid;
    std::string content;
    size_t docNum = 0;

#ifdef USE_TC_HASH
    LogServerStorage::ScdDbType::SDBCursor locn = scdDb->get_first_locn();
    while ( scdDb->get(locn, docid, content) )
    {
        docNum++;
        writeScdDoc(of, content, docid);
        scdDb->seq(locn);
    }
#else
//    if (scdDb->iterInit())
//    {
//        while (scdDb->iterNext(docid, content))
//        {
//            docNum++;
//            writeScdDoc(of, content, docid);
//        }
//    }
//    else
//    {
//        of.close();
//
//        response.success_ = false;
//        response.error_ = "LogServer error";
//        boost::filesystem::remove(filename);
//        return;
//    }

    LogServerStorage::ScdDbType::cursor_type cursor = scdDb->begin();
    while (scdDb->fetch(cursor, docid, content))
    {
        docNum++;
        writeScdDoc(of, content, docid);

        scdDb->iterNext(cursor);
    }
#endif

    scdStorage->isReIndexed_ = true;
    of.close();

    std::cout << "[Fetched doc number: " << docNum << "]" << std::endl;

    // no doc
    if (docNum > 0)
    {
        // transmit SCD file to required server, use rsync
        std::stringstream command;
        command << "rsync -vaz "
                << filename
                << " "
                << scdFileRequestData.username_
                << "@"
                << scdFileRequestData.host_
                << ":"
                << scdFileRequestData.path_
                << "/"; // ensure is dir

        std::cout << command.str() << std::endl;
        if (std::system(command.str().c_str()) == 0)
        {
            response.success_ = true;
            response.scdFileName_ = scdFileName;
        }
        else
        {
            response.success_ = false;
            response.error_ = "LogServer: rsync failed to synchronize SCD file.";
        }
    }
    else
    {
        response.success_ = false;
        response.error_ = "LogServer: SCD is empty.";
        boost::filesystem::remove(filename);
        return;
    }

    // remove local SCD file
    boost::filesystem::remove(filename);
}

void RpcLogServer::writeScdDoc(std::ofstream& of, const std::string& doc, const uint128_t& docid)
{
    std::string oldUuidStr;
    size_t startPos = doc.find(UUID_PROPERTY);
    if (startPos != std::string::npos)
    {
        startPos += UUID_PROPERTY.length();
        size_t endPos = doc.find_first_of("<\r\n", startPos);
        if (endPos != std::string::npos)
        {
            oldUuidStr = doc.substr(startPos, endPos-startPos);
        }
    }

    UUID2DocidList::DocidListType docidList;
    if (!oldUuidStr.empty())
    {
        try
        {
            uint128_t uuid = Utilities::uuidToUint128(oldUuidStr);
            LogServerStorage::get()->uuidDrum()->GetValue(uuid, docidList);
        }
        catch(const std::exception& e)
        {
            std::cout << "Uuid extraction error: " << e.what() << std::endl;
        }
    }

    if (docidList.empty())
    {
        // no need to update uuid (docid)
        of << doc;

#ifdef LOG_SERVER_DEBUG
        std::cout << "[Fetch doc]" << std::endl;
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
            of << newDoc;

#ifdef LOG_SERVER_DEBUG
            std::cout << "[Fetch doc]" << std::endl;
            std::cout << "[updated uuid: " << oldUuidStr << " -> " << newUuidStr << "]" << std::endl;
            std::cout << newDoc << std:: endl;
#endif
            // update once
            break;
        }
    }
}

void RpcLogServer::strIdToItemId_(msgpack::rpc::request& req)
{
    msgpack::type::tuple<StrIdToItemIdRequestData> params;
    req.params().convert(&params);
    const StrIdToItemIdRequestData& strIdToItemIdRequestData = params.get<0>();

    itemid_t itemId = 0;
    if (itemIdGenerator_.strIdToItemId(strIdToItemIdRequestData.collection_,
                                       strIdToItemIdRequestData.strId_,
                                       itemId))
    {
        req.result(itemId);
    }
    else
    {
        req.error(std::string("failed to convert from string id to item id"));
    }
}

void RpcLogServer::itemIdToStrId_(msgpack::rpc::request& req)
{
    msgpack::type::tuple<ItemIdToStrIdRequestData> params;
    req.params().convert(&params);
    const ItemIdToStrIdRequestData& itemIdToStrIdRequestData = params.get<0>();

    std::string strId;
    if (itemIdGenerator_.itemIdToStrId(itemIdToStrIdRequestData.collection_,
                                       itemIdToStrIdRequestData.itemId_,
                                       strId))
    {
        req.result(strId);
    }
    else
    {
        req.error(std::string("failed to convert from item id to string id"));
    }
}

void RpcLogServer::getMaxItemId_(msgpack::rpc::request& req)
{
    msgpack::type::tuple<std::string> params;
    req.params().convert(&params);
    const std::string& collection = params.get<0>();

    itemid_t itemId = itemIdGenerator_.maxItemId(collection);
    req.result(itemId);
}

}

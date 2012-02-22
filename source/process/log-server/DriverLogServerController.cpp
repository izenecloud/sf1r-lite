#include "DriverLogServerController.h"

#include <common/Utilities.h>
#include <common/ScdParser.h>

#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem.hpp>

namespace bfs = boost::filesystem;

namespace sf1r
{
using namespace izenelib::driver;
using namespace izenelib::net::sf1r;
using driver::Keys;


bool DriverLogServerController::preprocess()
{
    dirverLogServerHandler_->setRequestContext(request(), response());
    return true;
}

/**
 * @brief Action \b update_cclog
 *
 * - @b record (@c Object): a wrapped json request (as cclog) which is supported by SF1R
 *
 * @example
 * {
 *   "header" : {
 *     "controller" : "log_server"
 *     "action" : "update_cclog",
 *   },
 *   "host" : "172.16.0.161",
 *   "port" : "18181",
 *   "filename" : "cclog20120113.txt",
 *   "record" : {
 *       # a wrapped json request (as cclog) which is supported by SF1R
 *   }
 * }
 *
 */
void DriverLogServerController::update_cclog()
{
    //std::cout << request().controller() << "/" << request().action() << std::endl;
    dirverLogServerHandler_->processCclog();
}

/**
 * @brief Action \b update_cclog
 * The value of docid items is raw docid instead of uuid.
 *
 * - @b record (@c Object): a wrapped json request (as cclog) which is supported by SF1R
 *
 * @example
 * {
 *   "header" : {
 *     "controller" : "log_server"
 *     "action" : "update_cclog_rawid",
 *   },
 *   "host" : "172.16.0.161",
 *   "port" : "18181",
 *   "filename" : "cclog20120113.txt",
 *   "record" : {
 *       # a wrapped json request (as cclog) which is supported by SF1R
 *   }
 * }
 *
 */
void DriverLogServerController::convert_raw_cclog()
{
    //std::cout << request().controller() << "/" << request().action() << std::endl;
    dirverLogServerHandler_->processCclogRawid();
}

/**
 * @brief Action \b update_scd
 *
 * - @b record (@c String): a doc record in SCD
 * - @b DOCID (@c String): the docid of the doc record
 *
 * @example
 * {
 *   "header" : {
 *     "controller" : "log_server"
 *     "action" : "update_scd",
 *   },
 *   "filename" : "B-00-201112011437-38743-I-C.SCD",
 *   "DOCID" : "#{uuid_docid}"
 *   "record" : "#{the text of a record in SCD file}"
 *   }
 * }
 *
 */
void DriverLogServerController::update_scd()
{
    std::cout << request().controller() << "/" << request().action() << std::endl;
    dirverLogServerHandler_->processScd();
}

/**
 * @brief Action \b update_documents
 *
 * @example
 * {
 *   "header" : {
 *     "controller" : "log_server"
 *     "action" : "update_documents",
 *   },
 *   "host" : "172.16.0.163",
 *   "port" : "18181"
 *   "collection" : "example"
 *   }
 * }
 *
 */
void DriverLogServerController::update_documents()
{
    std::cout << request().controller() << "/" << request().action() << std::endl;
    dirverLogServerHandler_->processUpdateDocuments();
}

/**
 * @brief Action \b flush
 *
 * Client should send this request after all update requests for a file has been finished,
 * so that Server can perform flush properly.
 *
 * @example
 * {
 *   "header" : {
 *     "controller" : "log_server"
 *     "action" : "flush",
 *   },
 *   "filename" : "cclog20120113.txt"
 *   }
 * }
 *
 */
void DriverLogServerController::flush()
{
    std::cout << request().controller() << "/" << request().action() << std::endl;
    dirverLogServerHandler_->flush();
}

////////////////////////////////////////////////////////////////////////////////
/// DriverLogServerHandler

void DriverLogServerHandler::init()
{
    // call back for drum dispatcher
    LogServerStorage::get()->uuidDrumDispatcher().registerOp(
            LogServerStorage::UuidDrumDispatcherType::UNIQUE_KEY_CHECK,
            boost::bind(&DriverLogServerHandler::onUniqueKeyCheck, this, _1, _2, _3));

    LogServerStorage::get()->uuidDrumDispatcher().registerOp(
            LogServerStorage::UuidDrumDispatcherType::DUPLICATE_KEY_CHECK,
            boost::bind(&DriverLogServerHandler::onDuplicateKeyCheck, this, _1, _2, _3));

    // collections of which log need to be updated
    driverCollections_ = LogServerCfg::get()->getDriverCollections();
    storageBaseDir_ = LogServerCfg::get()->getStorageBaseDir();
    bfs::create_directories(storageBaseDir_+"/scd");
    bfs::create_directories(storageBaseDir_+"/cclog");
    bfs::create_directories(storageBaseDir_+"/cclog/converted");

    // cclogged requests which need to transmit back to SF1
    cclogRequestMap_.insert(std::make_pair("documents/log_group_label", true));
    cclogRequestMap_.insert(std::make_pair("recommend/add_user", true));
    cclogRequestMap_.insert(std::make_pair("recommend/update_user", true));
    cclogRequestMap_.insert(std::make_pair("recommend/remove_user", true));
}

void DriverLogServerHandler::processCclog()
{
    std::string host = asString(request()[Keys::host]);
    uint32_t port = asInt(request()[Keys::port]);

    if (host.empty())
    {
        response().setSuccess(false);
        response().addError("Sf1 host info is required!");
        std::cerr << "Sf1 host info is required!" << std::endl;
        return;
    }
    else
    {
        setCclogSf1DriverClient(host, port);
    }

    // Get raw request which was originally stored in cclog
    izenelib::driver::Value raw_request = request()[Keys::record];

    std::string json;
    jsonWriter_.write(raw_request, json);
    //std::cout << json << std::endl;

#ifdef OUTPUT_TO_FILE
    std::string fileName = storageBaseDir_ + "/cclog/" + asString(request()[Keys::filename]);

    if (!openFile(fileName))
    {
        std::string msg = "Server Error: Failed to create file " + fileName;
        response().setSuccess(false);
        response().addError(msg);
        std::cout << msg << std::endl;
        return;
    }
#endif

    const std::string& controller = asString(raw_request[Keys::header][Keys::controller]);
    const std::string& action = asString(raw_request[Keys::header][Keys::action]);
    std::string uri = controller + "/" + action;

#ifdef OUTPUT_TO_FILE
    uri = fileName;
#endif

    std::string collection = asString(raw_request[Keys::collection]);
    if (!skipProcess(collection))
    {
        if (controller == "documents" && action == "visit")
        {
            encodeFileName(json, uri);
            processDocVisit(raw_request, json);
        }
        else if (controller == "recommend" && action == "visit_item")
        {
            encodeFileName(json, uri);
            processRecVisitItem(raw_request, json);
        }
        else if (controller == "recommend" && action == "purchase_item")
        {
            encodeFileName(json, uri);
            processRecPurchaseItem(raw_request, json);
        }
        else
        {
            // output directly
            if (cclogRequestMap_.find(uri) != cclogRequestMap_.end())
            {
                outputCclog(uri, json);
            }
        }
    }
    else
    {
        // nothing to do
        //outputCclog(uri, json);
    }
}

void DriverLogServerHandler::processCclogRawid()
{
    std::string fileName = storageBaseDir_ + "/cclog/converted/" + asString(request()[Keys::filename]);
    if (!setConvertedCclogFile(fileName))
    {
        std::string msg = "Server Error: Failed to create file " + fileName;
        response().setSuccess(false);
        response().addError(msg);
        std::cout << msg << std::endl;
        return;
    }

    // Get raw request which was originally stored in cclog
    izenelib::driver::Value raw_request = request()[Keys::record];

    const std::string& controller = asString(raw_request[Keys::header][Keys::controller]);
    const std::string& action = asString(raw_request[Keys::header][Keys::action]);
    std::string uri = controller + "/" + action;

    std::string collection = asString(raw_request[Keys::collection]);
    if (collection == "b5mm")
    {
        collection = "b5ma";
        raw_request[Keys::collection] = "b5ma";
    }

    if (!skipProcess(collection))
    {
        std::string raw_json;
        jsonWriter_.write(raw_request, raw_json);

        if (cclogRequestMap_.find(uri) != cclogRequestMap_.end())
        {
            ouputConvertedCclog(raw_json);
        }
        else
        {
            if (controller == "documents" && action == "visit")
            {
                processDocVisitRawid(raw_request);
            }
            else if (controller == "recommend" && action == "visit_item")
            {
                processRecVisitItemRawid(raw_request);
            }
            else if (controller == "recommend" && action == "purchase_item")
            {
                processRecPurchaseItemRawid(raw_request);
            }
            else
            {
                return; // skip other requests
            }

            std::string converted_json;
            jsonWriter_.write(raw_request, converted_json);
            ouputConvertedCclog(converted_json);
        }
    }
}

void DriverLogServerHandler::processScd()
{
    std::string docid = asString(request()[Keys::DOCID]);
    std::string doc = asString(request()[Keys::record]);
    std::string fileName = storageBaseDir_ + "/scd/" + asString(request()[Keys::filename]);

    std::cout << fileName << " --> " << docid << std::endl;
    //std::cout << doc << std::endl;

    if (docid.empty())
    {
        response().setSuccess(false);
        response().addError("<DOCID> field is required!");
        return;
    }

    if (!openFile(fileName))
    {
        std::string msg = "Server Error: Failed to create file " + fileName;
        response().setSuccess(false);
        response().addError(msg);
        return;
    }

    try
    {
        boost::lock_guard<boost::mutex> lock(LogServerStorage::get()->uuidDrumMutex());
        encodeFileName(doc, fileName);
        LogServerStorage::get()->uuidDrum()->Check(Utilities::uuidToUint128(docid), doc);
    }
    catch (const std::exception& e)
    {
        std::string msg = e.what();
        msg += " (DOCID is excepted to be uuid generated by SF1R)";
        response().setSuccess(false);
        response().addError(msg);
    }
}

void DriverLogServerHandler::processUpdateDocuments()
{
    std::string host = asString(request()[Keys::host]);
    uint32_t port = asInt(request()[Keys::port]);
    std::string collection = asString(request()[Keys::collection]);

    std::cout << "[Update documents of " << collection << " to " << host << ":" << port << "]" << std::endl;

    if (collection.empty())
    {
        response().setSuccess(false);
        response().addError("collection field is required!");
        std::cerr << "collection field is required!" << std::endl;
        return;
    }

    if (!LogServerStorage::get()->checkScdDb(collection))
    {
        response().setSuccess(false);
        response().addError("LogServer: no SCD for required collection");
        return;
    }

    // sf1 driver client
    izenelib::net::sf1r::Sf1Config sf1Conf(1, false);
    izenelib::net::sf1r::Sf1Driver sf1DriverClient(host, port, sf1Conf);
    std::string uri = "documents/create";
    std::string tokens = "";

    boost::shared_ptr<LogServerStorage::ScdStorage>& scdStorage = LogServerStorage::get()->scdStorage(collection);
    boost::lock_guard<boost::mutex> lock(scdStorage->mutex_);

    // locad docs that comes after re-index
    ScdParser parser(izenelib::util::UString::UTF_8);
    if (!parser.load(scdStorage->scdFileName_))
    {
        response().setSuccess(false);
        response().addError("Could not Load Scd File.");
        std::cerr << "Could not Load Scd File." << std::endl;
        return;
    }

    std::string fieldStr;
    std::string fieldVal;
    for (ScdParser::iterator doc_iter = parser.begin();
        doc_iter != parser.end(); ++doc_iter)
    {
        izenelib::driver::Value requestValue;
        requestValue[Keys::collection] = collection;

        std::string docidStr;
        std::string docStr;

        vector<pair<izenelib::util::UString, izenelib::util::UString> >::iterator p;
        for (p = (*doc_iter)->begin(); p != (*doc_iter)->end(); p++)
        {
            p->first.convertString(fieldStr, izenelib::util::UString::UTF_8);
            p->second.convertString(fieldVal, izenelib::util::UString::UTF_8);

            if (fieldStr == "DOCID")
            {
                docidStr = fieldVal;
            }
            else if (fieldStr == "UUID")
            {
                //std::cout << fieldVal << " -> ";
                fieldVal = updateUuidStr(fieldVal);
                //std::cout << fieldVal << std::endl;
            }

            requestValue[Keys::resource][fieldStr] = fieldVal;
            docStr += "<" + fieldStr + ">" + fieldVal + "\n";
        }

        // save doc to scd db
        //std::cout << docidStr << std::endl;
        //std::cout << docStr << std::endl;
        scdStorage->scdDb_->update(Utilities::md5ToUint128(docidStr), docStr);

        // update doc to required sf1r server
        std::string requestString;
        jsonWriter_.write(requestValue, requestString);
        //std::cout << requestString << std::endl;

        string response = sf1DriverClient.call(uri, tokens, requestString);
        //std::cout << response << std::endl;
    }

    // reset flag
    scdStorage->isReIndexed_ = false;
    scdStorage->scdDb_->flush();

    // clear but not remove scd file
    scdStorage->scdFile_.close();
    scdStorage->scdFile_.open(scdStorage->scdFileName_.c_str());
}

void DriverLogServerHandler::flush()
{
    {
        boost::lock_guard<boost::mutex> lock(LogServerStorage::get()->uuidDrumMutex());
        LogServerStorage::get()->uuidDrum()->Synchronize();
    }

    {
        for (FileMapT::iterator it = fileMap_.begin(); it != fileMap_.end(); it++)
        {
            if (it->second)
            {
                boost::lock_guard<boost::mutex> lock(it->second->mutex_);
                it->second->of_->flush();
            }
        }
    }

    if (convertedCclogFile_)
        convertedCclogFile_->flush();
}


bool DriverLogServerHandler::skipProcess(const std::string& collection)
{
    if (driverCollections_.find(collection) == driverCollections_.end())
    {
        //std::cout << "skip " << collection << std::endl;
        return true;
    }

    return false;
}

/**
 * Process request of documents/visit
 * @param request
 * sample:
    { "collection" : "b5ma",
      "header" : { "acl_tokens" : "",
          "action" : "visit",
          "controller" : "documents",
          "remote_ip" : "127.0.0.1"
        },
      "resource" : { "DOCID" : "c119c900-1442-4aa0-bec2-a228cdc50d4c" }
    }
 *
 */
void DriverLogServerHandler::processDocVisit(izenelib::driver::Value& request, const std::string& raw)
{
    boost::lock_guard<boost::mutex> lock(LogServerStorage::get()->uuidDrumMutex());
    std::string docIdStr = asString(request[Keys::resource][Keys::DOCID]);
    LogServerStorage::get()->uuidDrum()->Check(Utilities::uuidToUint128(docIdStr), raw);
}

/**
 * Process request of recommend/visit_item
 * @param request
 * sample:
    { "collection" : "b5ma",
      "header" : { "acl_tokens" : "",
          "action" : "visit_item",
          "controller" : "recommend",
          "remote_ip" : "127.0.0.1"
        },
      "resource" : { "ITEMID" : "c1e72ce1-7649-402b-b349-7ca67ee6dd79",
          "USERID" : "",
          "is_recommend_item" : "false",
          "session_id" : ""
        }
    }
 *
 */
void DriverLogServerHandler::processRecVisitItem(izenelib::driver::Value& request, const std::string& raw)
{
    boost::lock_guard<boost::mutex> lock(LogServerStorage::get()->uuidDrumMutex());
    std::string itemIdStr = asString(request[Keys::resource][Keys::ITEMID]);
    LogServerStorage::get()->uuidDrum()->Check(Utilities::uuidToUint128(itemIdStr), raw);
}

/**
 * Process request of recommend/purchase_item
 * @param request
 * sample:
    { "collection" : "b5ma",
      "header" : { "acl_tokens" : "",
          "action" : "purchase_item",
          "controller" : "recommend",
          "remote_ip" : "127.0.0.1"
        },
      "resource" : { "USERID" : "",
          "items" : [ { "ITEMID" : "ad9c5415-b39a-4fe0-b882-3ca310aad7b8",
                "price" : 999,
                "quantity" : 1
              } ]
        }
    }
 *
 */
void DriverLogServerHandler::processRecPurchaseItem(izenelib::driver::Value& request, const std::string& raw)
{
    const Value& itemsValue = request[Keys::resource][Keys::items];
    if (nullValue(itemsValue) || itemsValue.type() != izenelib::driver::Value::kArrayType)
    {
        return;
    }

    boost::shared_ptr<CCLogMerge> cclogMergeUnit(new CCLogMerge(itemsValue.size(), raw));
    for (std::size_t i = 0; i < itemsValue.size(); ++i)
    {
        const Value& itemValue = itemsValue(i);
        std::string itemIdStr = asString(itemValue[Keys::ITEMID]);
        //std::cout << itemIdStr << "  ";
        uint128_t uuid = Utilities::uuidToUint128(itemIdStr);

        if (itemsValue.size() > 1) // XXX
        {
            boost::lock_guard<boost::mutex> lock(cclog_merge_mutex_);
            cclogMergeQueue_.insert(std::make_pair(uuid, cclogMergeUnit));
        }

        boost::lock_guard<boost::mutex> lock(LogServerStorage::get()->uuidDrumMutex());
        LogServerStorage::get()->uuidDrum()->Check(uuid, raw);
    }

    //std::cout << std::endl;
}

bool DriverLogServerHandler::processDocVisitRawid(izenelib::driver::Value& request)
{
    std::string docIdStr = asString(request[Keys::resource][Keys::DOCID]);  // raw docid
    uint128_t newUuid;

    boost::lock_guard<boost::mutex> lockDocidDrum(LogServerStorage::get()->docidDrumMutex());
    if (LogServerStorage::get()->docidDrum()->GetValue(Utilities::md5ToUint128(docIdStr), newUuid))
    {
        request[Keys::resource][Keys::DOCID] = Utilities::uint128ToUuid(newUuid);
        //boost::replace_all(raw, docIdStr, Utilities::uint128ToUuid(newUuid));
        return true;
    }

    return false;
}

bool DriverLogServerHandler::processRecVisitItemRawid(izenelib::driver::Value& request)
{
    std::string docIdStr = asString(request[Keys::resource][Keys::ITEMID]); // raw docid
    uint128_t newUuid;

    boost::lock_guard<boost::mutex> lockDocidDrum(LogServerStorage::get()->docidDrumMutex());
    if (LogServerStorage::get()->docidDrum()->GetValue(Utilities::md5ToUint128(docIdStr), newUuid))
    {
        request[Keys::resource][Keys::ITEMID] = Utilities::uint128ToUuid(newUuid);
        //boost::replace_all(raw, docIdStr, Utilities::uint128ToUuid(newUuid));
        return true;
    }

    return false;
}

bool DriverLogServerHandler::processRecPurchaseItemRawid(izenelib::driver::Value& request)
{
    Value& itemsValue = request[Keys::resource][Keys::items];
    if (nullValue(itemsValue) || itemsValue.type() != izenelib::driver::Value::kArrayType)
    {
        return false;
    }

    bool ret = false;
    for (std::size_t i = 0; i < itemsValue.size(); ++i)
    {
        Value& itemValue = itemsValue(i);
        std::string itemIdStr = asString(itemValue[Keys::ITEMID]); // raw docid
        //std::cout << itemIdStr << "  ";
        uint128_t docid = Utilities::md5ToUint128(itemIdStr);

        uint128_t newUuid;
        boost::lock_guard<boost::mutex> lock(LogServerStorage::get()->docidDrumMutex());
        if (LogServerStorage::get()->docidDrum()->GetValue(docid, newUuid))
        {
            itemValue[Keys::ITEMID] = Utilities::uint128ToUuid(newUuid);
            //boost::replace_all(raw, itemIdStr, Utilities::uint128ToUuid(newUuid));
            ret = true;
        }
    }

    return ret;
}

std::string DriverLogServerHandler::updateUuidStr(const std::string& uuidStr)
{
    boost::lock_guard<boost::mutex> lockUuidDrum(LogServerStorage::get()->uuidDrumMutex());
    boost::lock_guard<boost::mutex> lockDocidDrum(LogServerStorage::get()->docidDrumMutex());

    // check old uuid
    uint128_t oldUuid = Utilities::uuidToUint128(uuidStr);
    std::vector<uint128_t> docidList;
    LogServerStorage::get()->uuidDrum()->GetValue(oldUuid, docidList);

    // get new uuid
    uint128_t newUuid;
    std::vector<uint128_t>::iterator it;
    for (it = docidList.begin(); it != docidList.end(); ++it)
    {
        if (LogServerStorage::get()->docidDrum()->GetValue(*it, newUuid))
        {
            // update uuid
            return Utilities::uint128ToUuid(newUuid);
        }
    }

    return uuidStr;
}

void DriverLogServerHandler::onUniqueKeyCheck(
        const LogServerStorage::uuid_t& uuid,
        const LogServerStorage::raw_docid_list_t& docidList,
        const std::string& aux)
{
    //std::cout << "onUniqueKeyCheck " << Utilities::uint128ToUuid(uuid) << std::endl;
    std::string json = aux;
    std::string fileName;
    decodeFileName(json, fileName);
    outputCclog(fileName, json);
}

void DriverLogServerHandler::onDuplicateKeyCheck(
        const LogServerStorage::uuid_t& uuid,
        const LogServerStorage::raw_docid_list_t& docidList,
        const std::string& aux)
{
    //std::cout << "onDuplicateKeyCheck " << Utilities::uint128ToUuid(uuid) << std::endl;
    std::set<uint128_t> newUuidSet;
    uint128_t newUuid;
    for (std::size_t i = 0; i < docidList.size(); i++)
    {
        boost::lock_guard<boost::mutex> lock(LogServerStorage::get()->docidDrumMutex());
        if (LogServerStorage::get()->docidDrum()->GetValue(docidList[i], newUuid))
        {
            newUuidSet.insert(newUuid);
        }
    }

    std::map<uint128_t, boost::shared_ptr<CCLogMerge> >::iterator it = cclogMergeQueue_.find(uuid);
    if (it != cclogMergeQueue_.end())
    {
        boost::shared_ptr<CCLogMerge>& cclogMergeUnit = it->second;

        boost::lock_guard<boost::mutex> lock(cclog_merge_mutex_);
        cclogMergeUnit->uuidUpdateVec_.push_back(std::make_pair(uuid, newUuidSet));
        if (cclogMergeUnit->uuidCnt_ == cclogMergeUnit->uuidUpdateVec_.size())
            mergeCClog(cclogMergeUnit);
    }
    else
    {
        std::string oldUuidStr = Utilities::uint128ToUuid(uuid);
        std::string newUuidStr;
        for (std::set<uint128_t>::iterator sit = newUuidSet.begin();
                sit != newUuidSet.end(); ++sit)
        {
            newUuidStr = Utilities::uint128ToUuid(*sit);
            std::string log = aux;
            std::cout << oldUuidStr << " -> " << newUuidStr << std::endl;
            //std::cout << log;
            boost::replace_all(log, oldUuidStr, newUuidStr);
            //std::cout << " -> " << std::endl;
            //std::cout << "updated log: " << log << std::endl;

            std::string fileName;
            decodeFileName(log, fileName);
            outputCclog(fileName, log);
        }
    }
}

void DriverLogServerHandler::mergeCClog(boost::shared_ptr<CCLogMerge>& cclogMergeUnit)
{
    // join all uuids' update
    std::string& log = cclogMergeUnit->request_;

    std::vector<std::pair<uint128_t, std::set<uint128_t> > >::iterator it
        = cclogMergeUnit->uuidUpdateVec_.begin();
    for (; it != cclogMergeUnit->uuidUpdateVec_.end(); ++it)
    {
        std::string oldUuidStr = Utilities::uint128ToUuid(it->first);
        std::string newUuidStr = Utilities::uint128ToUuid(*(it->second.begin()));

        std::cout << oldUuidStr << " -> " << newUuidStr << std::endl;
        boost::replace_all(log, oldUuidStr, newUuidStr);
    }

    //std::cout << "updated log: " << log << std::endl;
    std::string fileName;
    decodeFileName(log, fileName);
    outputCclog(fileName, log);

    // remove merged
    for (it = cclogMergeUnit->uuidUpdateVec_.begin(); it != cclogMergeUnit->uuidUpdateVec_.end(); ++it)
    {
        cclogMergeQueue_.erase(it->first);
    }
}

boost::shared_ptr<DriverLogServerHandler::OutFile>& DriverLogServerHandler::openFile(const std::string& fileName)
{
    FileMapT::iterator it = fileMap_.find(fileName);

    if (it == fileMap_.end() || !it->second)
    {
        // create file if not existed
        std::ifstream inf;
        inf.open(fileName.c_str());
        if (!inf.good())
        {
            std::ifstream out;
            out.open(fileName.c_str());
            out.close();
        }
        inf.close();

        // open file for output
        boost::shared_ptr<OutFile> outFile(new OutFile);
        outFile->of_.reset(new std::ofstream);
        outFile->of_->open(fileName.c_str(), fstream::out|fstream::app);
        if (outFile->of_->good())
        {
            fileMap_[fileName] = outFile;
        }
        else
        {
            fileMap_[fileName].reset();
        }
    }

    return fileMap_[fileName];
}

void DriverLogServerHandler::encodeFileName(std::string& record, const std::string& fileName)
{
    record.append("}");
    record.append(fileName);
}

void DriverLogServerHandler::decodeFileName(std::string& record, std::string& fileName)
{
    std::size_t pos = record.find_last_of('}');
    if (pos != std::string::npos)
    {
        fileName = record.substr(pos+1);
        record.erase(pos);
    }
}

void DriverLogServerHandler::setCclogSf1DriverClient(const std::string& host, uint32_t port)
{
    if (!cclogSf1DriverClient_ || (cclogSf1Host_ != host || cclogSf1Port_ != port))
    {
        cclogSf1Host_ = host;
        cclogSf1Port_ = port;
        izenelib::net::sf1r::Sf1Config sf1Conf(1, false);
        cclogSf1DriverClient_.reset(new Sf1Driver(cclogSf1Host_, cclogSf1Port_, sf1Conf));
    }
}

boost::shared_ptr<Sf1Driver>& DriverLogServerHandler::getCclogSf1DriverClient()
{
    return cclogSf1DriverClient_;
}

void DriverLogServerHandler::outputCclog(const std::string& fileName, const std::string& request)
{
    static std::string tokens = "";
    std::string uri = fileName;

    //std::cout << "---> outputCclog : " << uri << " --- " << request << std::endl;

    boost::shared_ptr<Sf1Driver>& sf1DriverClient = getCclogSf1DriverClient();

    if (sf1DriverClient)
    {
        string requestString = request;
        string response = sf1DriverClient->call(uri, tokens, requestString);
    }

#ifdef OUTPUT_TO_FILE
    writeFile(fileName, request);
#else

#endif
}

void DriverLogServerHandler::writeFile(const std::string& fileName, const std::string& record)
{
    boost::shared_ptr<OutFile>& outFile = openFile(fileName);
    if (outFile)
    {
        boost::lock_guard<boost::mutex> lock(outFile->mutex_);
        (*outFile->of_) << record;
    }
    else
    {
        std::cerr << "Failed to write file: " << fileName << std::endl;
    }
}

bool DriverLogServerHandler::setConvertedCclogFile(const std::string& fileName)
{
    if (convertedCclogFileName_ != fileName)
    {
        convertedCclogFileName_ = fileName;
        convertedCclogFile_.reset(new std::ofstream(convertedCclogFileName_.c_str()));
    }

    if (!convertedCclogFile_)
    {
        convertedCclogFile_.reset(new std::ofstream(convertedCclogFileName_.c_str()));
    }

    return convertedCclogFile_->good();
}

void DriverLogServerHandler::ouputConvertedCclog(const std::string& request)
{
    if (convertedCclogFile_)
        (*convertedCclogFile_) << request;
}

}

#include "LogDispatchHandler.h"

#include <common/Utilities.h>
#include <common/ScdParser.h>

#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/bimap.hpp>
#include <boost/bimap/set_of.hpp>
#include <boost/bimap/multiset_of.hpp>

#include <glog/logging.h>

namespace bfs = boost::filesystem;

namespace sf1r
{
using namespace izenelib::driver;
using namespace izenelib::net::sf1r;
using driver::Keys;

////////////////////////////////////////////////////////////////////////////////
/// LogDispatchHandler

void LogDispatchHandler::Init()
{
    // call back for drum dispatcher
    LogServerStorage::get()->uuidDrumDispatcher().registerOp(
        LogServerStorage::UuidDrumDispatcherType::UNIQUE_KEY_CHECK,
        boost::bind(&LogDispatchHandler::OnUniqueKeyCheck, this, _1, _2, _3));

    LogServerStorage::get()->uuidDrumDispatcher().registerOp(
        LogServerStorage::UuidDrumDispatcherType::DUPLICATE_KEY_CHECK,
        boost::bind(&LogDispatchHandler::OnDuplicateKeyCheck, this, _1, _2, _3));

    // collections of which log need to be updated
    driverCollections_ = LogServerCfg::get()->getDriverCollections();
    storageBaseDir_ = LogServerCfg::get()->getStorageBaseDir();
    bfs::create_directories(storageBaseDir_+"/scd");
    bfs::create_directories(storageBaseDir_+"/cclog");
    bfs::create_directories(storageBaseDir_+"/cclog/history");
    bfs::create_directories(storageBaseDir_+"/cclog/history_raw");
    bfs::create_directories(storageBaseDir_+"/cclog/converted");

    // cclogged requests which need to transmit back to SF1
    cclogRequestMap_.insert(std::make_pair("documents/log_group_label", true));
    cclogRequestMap_.insert(std::make_pair("recommend/add_user", true));
    cclogRequestMap_.insert(std::make_pair("recommend/update_user", true));
    cclogRequestMap_.insert(std::make_pair("recommend/remove_user", true));
}

void LogDispatchHandler::DispatchLog()
{
    std::string host = asString(request()[Keys::host]);
    uint32_t port = asInt(request()[Keys::port]);
    if (host.empty())
    {
        response().setSuccess(false);
        response().addError("Sf1 host info is required!");
        LOG(ERROR) << "Sf1 host info is required!";
        return;
    }
    else
    {
        InitSf1DriverClient_(host, port);
    }

    bfs::path rawPath(asString(request()[Keys::filename]));
    std::string fileName = storageBaseDir_ + "/cclog/history/" + rawPath.filename().string();
    if (!OpenFile_(fileName))
    {
        LOG(WARNING)  << "Failed to create backup file: " << fileName;
    }

    izenelib::driver::Value raw_request = request()[Keys::record]; // request logged by cclog
    const std::string& controller = asString(raw_request[Keys::header][Keys::controller]);
    const std::string& action = asString(raw_request[Keys::header][Keys::action]);
    std::string uri = controller + "/" + action;

    std::string collection = asString(raw_request[Keys::collection]);

    if (!SkipProcess_(collection))
    {
        if (controller == "documents" && action == "visit")
        {
            DocVisit_(raw_request);
        }
        else if (controller == "recommend" && action == "visit_item")
        {
            RecVisitItem_(raw_request);
        }
        else if (controller == "recommend" && action == "purchase_item")
        {
            RecPurchaseItem_(raw_request);
        }
        else if (cclogRequestMap_.find(uri) != cclogRequestMap_.end())
        {
            // go ahead
        }
        else
        {
            // skip
            return;
        }

        std::string json;
        jsonWriter_.write(raw_request, json);
        OutputCclog_(fileName, uri, json);
    }
}

void LogDispatchHandler::BackupRawCclog()
{
    bfs::path rawPath(asString(request()[Keys::filename]));
    std::string fileName = storageBaseDir_ + "/cclog/history_raw/" + rawPath.stem().string() + ".raw";

    if (!OpenFile_(fileName))
    {
        std::string msg = "Server Error: Failed to create file " + fileName;
        response().setSuccess(false);
        response().addError(msg);
        LOG(ERROR) << msg;
        return;
    }

    izenelib::driver::Value raw_request = request()[Keys::record];
    const std::string& controller = asString(raw_request[Keys::header][Keys::controller]);
    const std::string& action = asString(raw_request[Keys::header][Keys::action]);
    std::string uri = controller + "/" + action;

    std::string collection = asString(raw_request[Keys::collection]);

    if (!SkipProcess_(collection))
    {
        if (controller == "documents" && action == "visit")
        {
            DocVisitBackupRawid_(raw_request);
        }
        else if (controller == "recommend" && action == "visit_item")
        {
            RecVisitItemBackupRawid_(raw_request);
        }
        else if (controller == "recommend" && action == "purchase_item")
        {
            RecPurchaseItemBackupRawid_(raw_request);
        }
        else if (cclogRequestMap_.find(uri) != cclogRequestMap_.end())
        {
            // go ahead
        }
        else
        {
            // skip
            return;
        }

        std::string request_json;
        jsonWriter_.write(raw_request, request_json);

        WriteFile_(fileName, request_json);
    }
}

void LogDispatchHandler::ConvertRawCclog()
{
    bfs::path rawPath(asString(request()[Keys::filename]));
    std::string fileName = storageBaseDir_ + "/cclog/converted/" + rawPath.stem().string() + ".log";
    if (!OpenFile_(fileName))
    {
        std::string msg = "Server Error: Failed to create file " + fileName;
        response().setSuccess(false);
        response().addError(msg);
        LOG(ERROR)  << msg;
        return;
    }

    // Get raw request which was originally stored in cclog
    izenelib::driver::Value raw_request = request()[Keys::record];

    const std::string& controller = asString(raw_request[Keys::header][Keys::controller]);
    const std::string& action = asString(raw_request[Keys::header][Keys::action]);
    std::string uri = controller + "/" + action;

    std::string collection = asString(raw_request[Keys::collection]);

    if (!SkipProcess_(collection))
    {
        if (controller == "documents" && action == "visit")
        {
            DocVisitConvertRawid_(raw_request);
        }
        else if (controller == "recommend" && action == "visit_item")
        {
            RecVisitItemConvertRawid_(raw_request);
        }
        else if (controller == "recommend" && action == "purchase_item")
        {
            RecPurchaseItemConvertRawid_(raw_request);
        }
        else if (cclogRequestMap_.find(uri) != cclogRequestMap_.end())
        {
            // go ahead
        }
        else
        {
            // skip
            return;
        }

        std::string request_json;
        jsonWriter_.write(raw_request, request_json);
        WriteFile_(fileName, request_json);
    }
}

void LogDispatchHandler::UpdateSCD()
{
    std::string docid = asString(request()[Keys::DOCID]);
    std::string doc = asString(request()[Keys::record]);
    bfs::path rawPath(asString(request()[Keys::filename]));
    std::string fileName = storageBaseDir_ + "/scd/" + rawPath.filename().string();

    LOG(INFO) << fileName << " --> " << docid;

    if (docid.empty())
    {
        response().setSuccess(false);
        response().addError("<DOCID> field is required!");
        return;
    }

    if (!OpenFile_(fileName))
    {
        std::string msg = "Server Error: Failed to create file " + fileName;
        response().setSuccess(false);
        response().addError(msg);
        return;
    }

    try
    {
        boost::lock_guard<boost::mutex> lock(LogServerStorage::get()->uuidDrumMutex());
        EncodeFileName_(doc, fileName);
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

void LogDispatchHandler::UpdateDocuments()
{
    std::string host = asString(request()[Keys::host]);
    uint32_t port = asInt(request()[Keys::port]);
    std::string collection = asString(request()[Keys::collection]);

    LOG(INFO) << "Update documents of " << collection
                    <<" to SF1R[" << host << ":" << port << "]";

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
    izenelib::net::sf1r::Sf1Config sf1Conf;
    std::string sf1Address = host + ":" + boost::lexical_cast<std::string>(port);
    izenelib::net::sf1r::Sf1Driver sf1DriverClient(sf1Address, sf1Conf);
    std::string uri = "documents/create";
    std::string tokens = "";

    boost::shared_ptr<LogServerStorage::ScdStorage>& scdStorage = 
        LogServerStorage::get()->scdStorage(collection);
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

    std::string fieldVal;
    for (ScdParser::iterator doc_iter = parser.begin();
            doc_iter != parser.end(); ++doc_iter)
    {
        izenelib::driver::Value requestValue;
        requestValue[Keys::collection] = collection;

        std::string docidStr;
        //std::string docStr;

        SCDDoc::iterator p = (*doc_iter)->begin();
        for (; p != (*doc_iter)->end(); p++)
        {
            const std::string& fieldName = p->first;
            std::string fieldNameLow = boost::algorithm::to_lower_copy(fieldName);
            fieldVal = p->second;

            if (fieldNameLow == "docid")
            {
                docidStr = fieldVal;
            }
            else if (fieldNameLow == "uuid")
            {
                LOG(INFO) << "old uuid" << fieldVal;
                fieldVal = UpdateUuidStr_(fieldVal);
            }

            requestValue[Keys::resource][fieldName] = fieldVal;
            //docStr += "<" + fieldName + ">" + fieldVal + "\n";
        }

        // save doc to scd db
        //scdStorage->scdDb_->update(Utilities::md5ToUint128(docidStr), docStr);

        // update doc to required sf1r server
        std::string requestString;
        jsonWriter_.write(requestValue, requestString);

        string response = sf1DriverClient.call(uri, tokens, requestString);
        LOG(INFO) << response;
    }

    // reset flag
    scdStorage->isReIndexed_ = false;
    //scdStorage->scdDb_->flush();
}

void LogDispatchHandler::Flush()
{
//    {
//        boost::lock_guard<boost::mutex> lock(LogServerStorage::get()->uuidDrumMutex());
//        LogServerStorage::get()->uuidDrum()->Synchronize();
//    }

    bfs::path rawPath(asString(request()[Keys::filename]));
    std::string action = asString(request()[Keys::action]);
    std::string fileName = storageBaseDir_ + "/cclog/";
    if (action == "update_cclog" || action == "update_cclog_history")
    {
        fileName += "history/" + rawPath.filename().string();
    }
    else if (action == "backup_raw_cclog")
    {
        fileName += "history_raw/" + rawPath.stem().string() + ".raw";
    }
    else if (action == "convert_raw_cclog")
    {
        fileName += "converted/" + rawPath.stem().string() + ".log";
    }
    else
    {
        // flush all
    }
    LOG(INFO)  << "flush " << fileName;

    FileMapT::iterator it = fileMap_.find(fileName);
    if (it != fileMap_.end())
    {
        if (it->second)
        {
            boost::lock_guard<boost::mutex> lock(it->second->mutex_);
            it->second->of_->flush();
            it->second->of_->close();
        }
    }
    else
    {
        LOG(INFO)  << "not found " << fileName;
    }

//    {
//        for (FileMapT::iterator it = fileMap_.begin(); it != fileMap_.end(); it++)
//        {
//            if (it->second)
//            {
//                boost::lock_guard<boost::mutex> lock(it->second->mutex_);
//                it->second->of_->flush();
//            }
//        }
//    }

//    if (convertedCclogFile_)
//        convertedCclogFile_->flush();
}


bool LogDispatchHandler::SkipProcess_(const std::string& collection)
{
    if (driverCollections_.find(collection) == driverCollections_.end())
    {
        return true;
    }

    return false;
}

/**
 * Process request of documents/visit
 * @param request
 * sample:
    { "collection" : "b5mp",
      "header" : { "acl_tokens" : "",
          "action" : "visit",
          "controller" : "documents",
          "remote_ip" : "127.0.0.1"
        },
      "resource" : { "DOCID" : "c119c900-1442-4aa0-bec2-a228cdc50d4c" }
    }
 *
 */
void LogDispatchHandler::DocVisit_(izenelib::driver::Value& request)
{
    std::string oldUuidStr = asString(request[Keys::resource][Keys::DOCID]);
    std::vector<uint128_t> docidList;

    boost::lock_guard<boost::mutex> lock(LogServerStorage::get()->uuidDrumMutex());
    if (LogServerStorage::get()->uuidDrum()->GetValue(Utilities::uuidToUint128(oldUuidStr), docidList))
    {
        std::vector<uint128_t> newUuids;
        if (GetUuidByDocidList_(docidList, newUuids))
        {
            LOG(INFO) << oldUuidStr << "  -->  " << Utilities::uint128ToUuid(newUuids[0]);
            request[Keys::resource][Keys::DOCID] = Utilities::uint128ToUuid(newUuids[0]);
        }
    }
}

/**
 * Process request of recommend/visit_item
 * @param request
 * sample:
    { "collection" : "b5mp",
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
void LogDispatchHandler::RecVisitItem_(izenelib::driver::Value& request)
{
    std::string oldUuidStr = asString(request[Keys::resource][Keys::ITEMID]);
    std::vector<uint128_t> docidList;

    boost::lock_guard<boost::mutex> lock(LogServerStorage::get()->uuidDrumMutex());
    if (LogServerStorage::get()->uuidDrum()->GetValue(Utilities::uuidToUint128(oldUuidStr), docidList))
    {
        std::vector<uint128_t> newUuids;
        if (GetUuidByDocidList_(docidList, newUuids))
        {
            LOG(INFO) << oldUuidStr << "  -->  " << Utilities::uint128ToUuid(newUuids[0]);
            request[Keys::resource][Keys::ITEMID] = Utilities::uint128ToUuid(newUuids[0]);
        }
    }
}

/**
 * Process request of recommend/purchase_item
 * @param request
 * sample:
    { "collection" : "b5mp",
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
void LogDispatchHandler::RecPurchaseItem_(izenelib::driver::Value& request)
{
    Value& itemsValue = request[Keys::resource][Keys::items];
    if (nullValue(itemsValue) || itemsValue.type() != izenelib::driver::Value::kArrayType)
    {
        return;
    }

    for (std::size_t i = 0; i < itemsValue.size(); ++i)
    {
        Value& itemValue = itemsValue(i);
        std::string oldUuidStr = asString(itemValue[Keys::ITEMID]);
        std::vector<uint128_t> docidList;

        boost::lock_guard<boost::mutex> lock(LogServerStorage::get()->uuidDrumMutex());
        if (LogServerStorage::get()->uuidDrum()->GetValue(Utilities::uuidToUint128(oldUuidStr), docidList))
        {
            std::vector<uint128_t> newUuids;
            if (GetUuidByDocidList_(docidList, newUuids))
            {
                LOG(INFO) << oldUuidStr << "  -->  " << Utilities::uint128ToUuid(newUuids[0]);
                itemValue[Keys::ITEMID] = Utilities::uint128ToUuid(newUuids[0]);
            }
        }
    }
}

void LogDispatchHandler::RecPurchaseItem_(izenelib::driver::Value& request, const std::string& raw)
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
        uint128_t uuid = Utilities::uuidToUint128(itemIdStr);

        if (itemsValue.size() > 1) // XXX
        {
            boost::lock_guard<boost::mutex> lock(cclog_merge_mutex_);
            cclogMergeQueue_.insert(std::make_pair(uuid, cclogMergeUnit));
        }

        boost::lock_guard<boost::mutex> lock(LogServerStorage::get()->uuidDrumMutex());
        LogServerStorage::get()->uuidDrum()->Check(uuid, raw);
    }
}

void LogDispatchHandler::DocVisitBackupRawid_(izenelib::driver::Value& request)
{
    std::string uuidStr = asString(request[Keys::resource][Keys::DOCID]);  // uuid
    std::vector<uint128_t> rawDocidList;

    boost::lock_guard<boost::mutex> lockUuidDrum(LogServerStorage::get()->uuidDrumMutex());
    if (LogServerStorage::get()->uuidDrum()->GetValue(Utilities::uuidToUint128(uuidStr), rawDocidList))
    {
        if (rawDocidList.size() > 0)
            request[Keys::resource][Keys::DOCID] = Utilities::uint128ToMD5(rawDocidList[0]);
    }
}

void LogDispatchHandler::RecVisitItemBackupRawid_(izenelib::driver::Value& request)
{
    std::string uuidStr = asString(request[Keys::resource][Keys::ITEMID]); // uuid
    std::vector<uint128_t> rawDocidList;

    boost::lock_guard<boost::mutex> lockUuidDrum(LogServerStorage::get()->uuidDrumMutex());
    if (LogServerStorage::get()->uuidDrum()->GetValue(Utilities::uuidToUint128(uuidStr), rawDocidList))
    {
        if (rawDocidList.size() > 0)
            request[Keys::resource][Keys::ITEMID] = Utilities::uint128ToMD5(rawDocidList[0]);
    }
}

void LogDispatchHandler::RecPurchaseItemBackupRawid_(izenelib::driver::Value& request)
{
    Value& itemsValue = request[Keys::resource][Keys::items];
    if (nullValue(itemsValue) || itemsValue.type() != izenelib::driver::Value::kArrayType)
    {
        return;
    }

    for (std::size_t i = 0; i < itemsValue.size(); ++i)
    {
        Value& itemValue = itemsValue(i);
        std::string uuidStr = asString(itemValue[Keys::ITEMID]); // uuid
        std::vector<uint128_t> rawDocidList;

        boost::lock_guard<boost::mutex> lockUuidDrum(LogServerStorage::get()->uuidDrumMutex());
        if (LogServerStorage::get()->uuidDrum()->GetValue(Utilities::uuidToUint128(uuidStr), rawDocidList))
        {
            if (rawDocidList.size() > 0)
                itemValue[Keys::ITEMID] = Utilities::uint128ToMD5(rawDocidList[0]);
        }
    }
}


bool LogDispatchHandler::DocVisitConvertRawid_(izenelib::driver::Value& request)
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

bool LogDispatchHandler::RecVisitItemConvertRawid_(izenelib::driver::Value& request)
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

bool LogDispatchHandler::RecPurchaseItemConvertRawid_(izenelib::driver::Value& request)
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

std::string LogDispatchHandler::UpdateUuidStr_(const std::string& uuidStr)
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

///Get suitable UUID according to DOCID list.
///The policy is, return the most frequent UUID
bool LogDispatchHandler::GetUuidByDocidList_(
    const std::vector<uint128_t>& docidList, 
    std::vector<uint128_t>& uuids)
{
    boost::lock_guard<boost::mutex> lockDocidDrum(LogServerStorage::get()->docidDrumMutex());

    using namespace boost::bimaps;
    typedef boost::bimap<set_of<uint128_t>, multiset_of<unsigned> > uuid_freq_map_type;
 
    bool ret = false;
    uuid_freq_map_type uuid_freq_map;

    std::vector<uint128_t>::const_iterator it;
    for (it = docidList.begin(); it != docidList.end(); ++it)
    {
        uint128_t uuid;
        if (LogServerStorage::get()->docidDrum()->GetValue(*it, uuid))
        {
            ret = true;
            uuid_freq_map_type::left_map::const_iterator uuid_left_it = uuid_freq_map.left.find(uuid);
            if(uuid_left_it == uuid_freq_map.left.end())
                uuid_freq_map.left.insert(uuid_freq_map_type::left_map::value_type(uuid, 1));
            else
            {
                unsigned freq = uuid_left_it->second + 1;
                uuid_freq_map.left.erase(uuid);
                uuid_freq_map.left.insert(uuid_freq_map_type::left_map::value_type(uuid, freq));
            }
        }
    }

    if(ret)
    {
        uuid_freq_map_type::right_map::const_reverse_iterator iter = uuid_freq_map.right.rbegin();
        uuids.push_back(iter->second);
    }

    return ret;
}

void LogDispatchHandler::OnUniqueKeyCheck(
    const LogServerStorage::uuid_t& uuid,
    const LogServerStorage::raw_docid_list_t& docidList,
    const std::string& aux)
{
    std::string json = aux;
    std::string fileName;
    DecodeFileName_(json, fileName);
    ///OutputCclog_(fileName, json);
}

void LogDispatchHandler::OnDuplicateKeyCheck(
    const LogServerStorage::uuid_t& uuid,
    const LogServerStorage::raw_docid_list_t& docidList,
    const std::string& aux)
{
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
            MergeCClog_(cclogMergeUnit);
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
            LOG(INFO) << oldUuidStr << " -> " << newUuidStr;
            //std::cout << log;
            boost::replace_all(log, oldUuidStr, newUuidStr);
            //std::cout << " -> " << std::endl;
            //std::cout << "updated log: " << log << std::endl;

            std::string fileName;
            DecodeFileName_(log, fileName);
            ///OutputCclog_(fileName, log);
        }
    }
}

void LogDispatchHandler::MergeCClog_(
    boost::shared_ptr<CCLogMerge>& cclogMergeUnit)
{
    // join all uuids' update
    std::string& log = cclogMergeUnit->request_;

    std::vector<std::pair<uint128_t, std::set<uint128_t> > >::iterator it
    = cclogMergeUnit->uuidUpdateVec_.begin();
    for (; it != cclogMergeUnit->uuidUpdateVec_.end(); ++it)
    {
        std::string oldUuidStr = Utilities::uint128ToUuid(it->first);
        std::string newUuidStr = Utilities::uint128ToUuid(*(it->second.begin()));

        LOG(INFO) << oldUuidStr << " -> " << newUuidStr;
        boost::replace_all(log, oldUuidStr, newUuidStr);
    }

    //std::cout << "updated log: " << log << std::endl;
    std::string fileName;
    DecodeFileName_(log, fileName);
    ///OutputCclog_(fileName, log);

    // remove merged
    it = cclogMergeUnit->uuidUpdateVec_.begin();
    for (; it != cclogMergeUnit->uuidUpdateVec_.end(); ++it)
    {
        cclogMergeQueue_.erase(it->first);
    }
}

boost::shared_ptr<LogDispatchHandler::OutFile>& 
LogDispatchHandler::OpenFile_(const std::string& fileName)
{
    FileMapT::iterator it = fileMap_.find(fileName);
    if (it == fileMap_.end())
    {
        // open file for output, overwrite
        boost::shared_ptr<OutFile> outFile(new OutFile);
        outFile->of_.reset(new std::ofstream);
        outFile->of_->open(fileName.c_str());
        if (outFile->of_->good())
        {
            fileMap_[fileName] = outFile;
        }
        else
        {
            fileMap_[fileName].reset();
        }
    }
    else
    {
        if (it->second)
        {
            boost::lock_guard<boost::mutex> lock(it->second->mutex_);
            if (!it->second->of_->is_open())
            {
                LOG(INFO) << "reopen " << fileName;
                it->second->of_->open(fileName.c_str());
            }
        }
    }

    return fileMap_[fileName];
}

void LogDispatchHandler::EncodeFileName_(
    std::string& record, 
    const std::string& fileName)
{
    record.append("}");
    record.append(fileName);
}

void LogDispatchHandler::DecodeFileName_(
    std::string& record, 
    std::string& fileName)
{
    std::size_t pos = record.find_last_of('}');
    if (pos != std::string::npos)
    {
        fileName = record.substr(pos+1);
        record.erase(pos);
    }
}

void LogDispatchHandler::InitSf1DriverClient_(
    const std::string& host, 
    uint32_t port)
{
    if (!sf1DriverClient_ || (sf1Host_ != host || sf1Port_ != port))
    {
        sf1Host_ = host;
        sf1Port_ = port;
        izenelib::net::sf1r::Sf1Config sf1Conf;
        std::string sf1Address = host + ":" + boost::lexical_cast<std::string>(port);
        sf1DriverClient_.reset(new Sf1Driver(sf1Address, sf1Conf));
    }
}

void LogDispatchHandler::OutputCclog_(
    const std::string& fileName, 
    const std::string& uri, 
    const std::string& request)
{
    static std::string tokens = "";

    boost::shared_ptr<Sf1Driver>& sf1DriverClient = sf1DriverClient_;
    if (sf1DriverClient)
    {
        string requestString = request;
        string response = sf1DriverClient->call(uri, tokens, requestString);
        LOG(INFO) << response;
    }

    WriteFile_(fileName, request); //xxx
}

void LogDispatchHandler::WriteFile_(
    const std::string& fileName, 
    const std::string& record)
{
    boost::shared_ptr<OutFile>& outFile = OpenFile_(fileName);
    if (outFile)
    {
        boost::lock_guard<boost::mutex> lock(outFile->mutex_);
        (*outFile->of_) << record;
    }
    else
    {
        LOG(ERROR) << "Failed to write file: " << fileName;
    }
}

}

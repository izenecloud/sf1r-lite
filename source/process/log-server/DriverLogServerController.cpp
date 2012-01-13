#include "DriverLogServerController.h"

#include <common/Utilities.h>

#include <boost/algorithm/string/replace.hpp>


namespace sf1r
{
using namespace izenelib::driver;
using driver::Keys;


bool DriverLogServerController::preprocess()
{
    dirverLogServerHandler_->setRequestContext(request(), response());
    return true;
}

/**
 * @brief Action \b update_cclog
 *
 * - @b line (@c Object): a wrapped json request (as cclog) which is supported by SF1R
 *
 * @example
 * {
 *   "header" : {
 *     "controller" : "log_server"
 *     "action" : "update_cclog",
 *   },
 *   "filename" : "cclog20120113.txt",
 *   "line" : {
 *     # a wrapped json request (as cclog) which is supported by SF1R
 *   }
 * }
 *
 */
void DriverLogServerController::update_cclog()
{
    std::cout << request().controller() << "/" << request().action() << std::endl;
    dirverLogServerHandler_->processCclog();
}

/**
 * @brief Action \b update_cclog
 *
 * - @b line (@c String): the text of a record in SCD file
 *
 * @example
 * {
 *   "header" : {
 *     "controller" : "log_server"
 *     "action" : "update_scd",
 *   },
 *   "filename" : "B-00-201112011437-38743-I-C.SCD",
 *   "line" : "#{the text of a record in SCD file}"
 *   }
 * }
 *
 */
void DriverLogServerController::update_scd()
{
    std::cout << request().controller() << "/" << request().action() << std::endl;
    // TODO
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
 *   "filename" : "B-00-201112011437-38743-I-C.SCD"
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
    LogServerStorage::get()->drumDispatcher().registerOp(
            LogServerStorage::DrumDispatcherImpl::UNIQUE_KEY_CHECK,
            boost::bind(&DriverLogServerHandler::onUniqueKeyCheck, this, _1, _2, _3));

    LogServerStorage::get()->drumDispatcher().registerOp(
            LogServerStorage::DrumDispatcherImpl::DUPLICATE_KEY_CHECK,
            boost::bind(&DriverLogServerHandler::onDuplicateKeyCheck, this, _1, _2, _3));

    // collections of which log need to be updated
    driverCollections_ = LogServerCfg::get()->getDriverCollections();

    // cclog file for output
    cclogFileName_ = LogServerCfg::get()->getCclogOutFile();
    {
        std::ifstream inf;
        inf.open(cclogFileName_.c_str());
        if (!inf.good())
        {
            // create if not existed
            std::ifstream out;
            out.open(cclogFileName_.c_str());
            out.close();
        }
        inf.close();
    }
    cclogFile_.reset(new std::ofstream);
    cclogFile_->open(cclogFileName_.c_str(), fstream::out/* | fstream::app*/); //FIXME
    if (!cclogFile_->good())
    {
        std::string msg = "Failed to open cclog file for output: " + cclogFileName_;
        throw std::runtime_error(msg.c_str());
    }
}

void DriverLogServerHandler::processCclog()
{
    // Get raw request which was originally stored in cclog
    izenelib::driver::Value raw_request = request()[Keys::line];
    std::string fileName = asString(request()[Keys::filename]);

    std::string raw;
    jsonWriter_.write(raw_request, raw);
    std::cout << raw << std::endl;

    std::string collection = asString(raw_request[Keys::collection]);
    if (!skipProcess(collection))
    {
        const std::string& controller = asString(raw_request[Keys::header][Keys::controller]);
        const std::string& action = asString(raw_request[Keys::header][Keys::action]);

        if (controller == "documents" && action == "visit")
        {
            processDocVisit(raw_request, raw);
        }
        else if (controller == "recommend" && action == "visit_item")
        {
            processRecVisitItem(raw_request, raw);
        }
        else if (controller == "recommend" && action == "purchase_item")
        {
            processRecPurchaseItem(raw_request, raw);
        }
        else
        {
            // output directly if there is no need to update
            ouputCclog(raw);
        }
    }
    else
    {
        // output directly if there is no need to update
        ouputCclog(raw);
    }
}

void DriverLogServerHandler::processScd()
{

}

void DriverLogServerHandler::flush()
{
    {
        boost::lock_guard<boost::mutex> lock(LogServerStorage::get()->drumMutex());
        LogServerStorage::get()->drum()->Synchronize();
    }

    {
        boost::lock_guard<boost::mutex> lock(cclog_file_mutex_);
        cclogFile_->flush();
    }
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
    boost::lock_guard<boost::mutex> lock(LogServerStorage::get()->drumMutex());
    std::string docIdStr = asString(request[Keys::resource][Keys::DOCID]);
    LogServerStorage::get()->drum()->Check(Utilities::uuidToUint128(docIdStr), raw);
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
    boost::lock_guard<boost::mutex> lock(LogServerStorage::get()->drumMutex());
    std::string itemIdStr = asString(request[Keys::resource][Keys::ITEMID]);
    LogServerStorage::get()->drum()->Check(Utilities::uuidToUint128(itemIdStr), raw);
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
    //std::cout << "RecPurchaseItemProcessor ";

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

        boost::lock_guard<boost::mutex> lock(LogServerStorage::get()->drumMutex());
        LogServerStorage::get()->drum()->Check(uuid, raw);
    }

    //std::cout << std::endl;
}

void DriverLogServerHandler::onUniqueKeyCheck(
        const LogServerStorage::drum_key_t& uuid,
        const LogServerStorage::drum_value_t& docidList,
        const LogServerStorage::drum_aux_t& aux)
{
    //std::cout << "onUniqueKeyCheck " << Utilities::uint128ToUuid(uuid) << std::endl;
    ouputCclog(aux);
}

void DriverLogServerHandler::onDuplicateKeyCheck(
        const LogServerStorage::drum_key_t& uuid,
        const LogServerStorage::drum_value_t& docidList,
        const LogServerStorage::drum_aux_t& aux)
{
    //std::cout << "onDuplicateKeyCheck " << Utilities::uint128ToUuid(uuid) << std::endl;
    std::set<uint128_t> newUuidSet;
    uint128_t newUuid;
    for (std::size_t i = 0; i < docidList.size(); i++)
    {
        boost::lock_guard<boost::mutex> lock(LogServerStorage::get()->docidDBMutex());
        if (LogServerStorage::get()->docidDB()->get(docidList[i], newUuid))
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
            std::cout << log;
            boost::replace_all(log, oldUuidStr, newUuidStr);
            std::cout << " -> " << std::endl;
            std::cout << log << std::endl;

            ouputCclog(log);
        }
    }
}

void DriverLogServerHandler::ouputCclog(const std::string& log)
{
    boost::lock_guard<boost::mutex> lock(cclog_file_mutex_);
    (*cclogFile_) << log;
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

    std::cout << "merged log: " << log << std::endl;
    ouputCclog(log);

    // remove merged
    for (it = cclogMergeUnit->uuidUpdateVec_.begin(); it != cclogMergeUnit->uuidUpdateVec_.end(); ++it)
    {
        cclogMergeQueue_.erase(it->first);
    }
}

}

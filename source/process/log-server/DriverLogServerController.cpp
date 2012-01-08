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
    BOOST_ASSERT(request_);
    BOOST_ASSERT(response_);
    return true;
}

/**
 * @brief Action \b update_cclog
 * The request data of \b update_cclog in json format is expected supported by any action of SF1R controllers,
 * and the \b header field is presented.
 *
 * @example
 * {
 *   "header" : {
 *     "action" : "documents",
 *     "controller" : "visit"
 *   },
 *
 *   # Following by other fields (this is comment)
 * }
 *
 */
void DriverLogServerController::update_cclog()
{
    dirverLogServerHandler_->process();
}


void DriverLogServerHandler::init()
{
    // drum <uuid, docids>
    drum_ = LogServerStorage::get()->getDrum();

    LogServerStorage::get()->getDrumDispatcher().registerOp(
            LogServerStorage::DrumDispatcherImpl::UNIQUE_KEY_CHECK,
            boost::bind(&DriverLogServerHandler::onUniqueKeyCheck, this, _1, _2, _3));

    LogServerStorage::get()->getDrumDispatcher().registerOp(
            LogServerStorage::DrumDispatcherImpl::DUPLICATE_KEY_CHECK,
            boost::bind(&DriverLogServerHandler::onDuplicateKeyCheck, this, _1, _2, _3));

    // DB <docid, uuid>
    docidDB_ = LogServerStorage::get()->getDocidDB();

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

void DriverLogServerHandler::process()
{
    std::cout << request().controller() << "/" << request().action() << std::endl;

    std::string collection = asString(request()["collection"]);

    std::string raw;
    jsonWriter_.write(request().get(), raw);

    if (!skipProcess(collection))
    {
        const std::string& controller = request().controller();
        const std::string& action = request().action();

        if (controller == "documents" && action == "visit")
        {
            processDocVisit(request(), raw);
        }
        else if (controller == "recommend" && action == "visit_item")
        {
            processRecVisitItem(request(), raw);
        }
        else if (controller == "recommend" && action == "purchase_item")
        {
            processRecPurchaseItem(request(), raw);
        }

        // xxx will be processed later asynchronously
        // flush drum properly
        drum_->Synchronize();
        return;
    }

    // output
    ouputCclog(raw);
}


bool DriverLogServerHandler::skipProcess(const std::string& collection)
{
    if (driverCollections_.find(collection) == driverCollections_.end())
    {
        std::cout << "skip " << collection << std::endl;
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
void DriverLogServerHandler::processDocVisit(izenelib::driver::Request& request, const std::string& raw)
{
    std::string docIdStr = asString(request[Keys::resource][Keys::DOCID]);
    drum_->Check(Utilities::uuidToUint128(docIdStr), raw);
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
void DriverLogServerHandler::processRecVisitItem(izenelib::driver::Request& request, const std::string& raw)
{
    std::string itemIdStr = asString(request[Keys::resource][Keys::ITEMID]);
    drum_->Check(Utilities::uuidToUint128(itemIdStr), raw);
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
void DriverLogServerHandler::processRecPurchaseItem(izenelib::driver::Request& request, const std::string& raw)
{
    //std::cout << "RecPurchaseItemProcessor ";

    const Value& itemsValue = request[Keys::resource][Keys::items];
    if (nullValue(itemsValue) || itemsValue.type() != izenelib::driver::Value::kArrayType)
    {
        return;
    }

    boost::shared_ptr<CCLogMerge> cclogMergeUnit(new CCLogMerge);
    cclogMergeUnit->request_ = raw;
    for (std::size_t i = 0; i < itemsValue.size(); ++i)
    {
        const Value& itemValue = itemsValue(i);
        std::string itemIdStr = asString(itemValue[Keys::ITEMID]);
        //std::cout << itemIdStr << "  ";
        uint128_t uuid = Utilities::uuidToUint128(itemIdStr);

        if (itemsValue.size() > 1)
        {
            // In case a request contain more than one uuid,
            // we have to update all these uuids together for this same request.
            // cclogMergeUnit is used to join all uuids' update.
            cclogMergeUnit->uuidCnt_ ++;

            boost::lock_guard<boost::mutex> lock(cclog_merge_mutex_);
            cclogMergeQueue_.insert(std::make_pair(uuid, cclogMergeUnit));
        }

        drum_->Check(uuid, raw);
    }

    //std::cout << std::endl;
}

void DriverLogServerHandler::onUniqueKeyCheck(
        const uint128_t& uuid,
        const std::vector<uint32_t>& docidList,
        const std::string& aux)
{
    //std::cout << "onUniqueKeyCheck " << Utilities::uint128ToUuid(uuid) << std::endl;
    ouputCclog(aux);
}

void DriverLogServerHandler::onDuplicateKeyCheck(
        const uint128_t& uuid,
        const std::vector<uint32_t>& docidList,
        const std::string& aux)
{
    std::cout << "onDuplicateKeyCheck " << Utilities::uint128ToUuid(uuid) << std::endl;

    std::set<uint128_t> newUuidSet;
    uint128_t newUuid;
    for (std::size_t i = 0; i < docidList.size(); i++)
    {
        if (docidDB_->get(docidList[i], newUuid))
        {
            newUuidSet.insert(newUuid);
            //std::cout << docidList[i] << " -> " << Utilities::uint128ToUuid(newUuid) << std::endl;
        }
    }

    // Check if uuid is one of uuids for the raw request (need to join uuids' update later),
    // or else the request contain only one uuid (just output to log).
    boost::lock_guard<boost::mutex> lock(cclog_merge_mutex_);
    std::map<uint128_t, boost::shared_ptr<CCLogMerge> >::iterator it;
    it = cclogMergeQueue_.find(uuid);
    if (it != cclogMergeQueue_.end())
    {
        boost::shared_ptr<CCLogMerge>& cclogMergeUnit = it->second;
        cclogMergeUnit->uuidUpdateVec_.push_back(std::make_pair(uuid, newUuidSet));
        mergeCClog(); // xxx
    }
    else
    {
        std::string oldUuidStr = Utilities::uint128ToUuid(uuid);
        std::string newUuidStr;
        std::set<uint128_t>::iterator it;
        for (it = newUuidSet.begin(); it != newUuidSet.end(); it++)
        {
            newUuidStr = Utilities::uint128ToUuid(*it);
            std::string log = aux;
            std::cout << oldUuidStr << " -> " <<newUuidStr << std::endl;
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
    boost::lock_guard<boost::mutex> lock(mutex_);

    (*cclogFile_) << log;
    cclogFile_->flush(); // xxx
}

void DriverLogServerHandler::mergeCClog()
{
    std::map<uint128_t, boost::shared_ptr<CCLogMerge> >::iterator it;
    for (it = cclogMergeQueue_.begin(); it != cclogMergeQueue_.end(); it++)
    {
        boost::shared_ptr<CCLogMerge>& cclogMergeUnit = it->second;
        if (cclogMergeUnit->merged_ == true)
        {
            continue;
        }

        if (cclogMergeUnit->uuidCnt_ == cclogMergeUnit->uuidUpdateVec_.size())
        {
            // join all uuids' update
            std::string log = cclogMergeUnit->request_;
            //std::cout<<"merge log: "<<log <<std::endl;

            std::vector<std::pair<uint128_t, std::set<uint128_t> > >::iterator itv;
            for (itv = cclogMergeUnit->uuidUpdateVec_.begin(); itv != cclogMergeUnit->uuidUpdateVec_.end(); itv++)
            {
                std::string oldUuidStr = Utilities::uint128ToUuid(itv->first);

                std::string newUuidStr;
                std::set<uint128_t>& newUuidSet = itv->second;
                std::set<uint128_t>::iterator its;
                for (its = newUuidSet.begin(); its != newUuidSet.end(); its++)
                {
                    // use 1st uuid
                    newUuidStr = Utilities::uint128ToUuid(uint128_t(*its));
                    break;
                }

                std::cout << oldUuidStr << " -> " <<newUuidStr << std::endl;
                boost::replace_all(log, oldUuidStr, newUuidStr);
            }

            std::cout<<"merged log: "<<log <<std::endl;
            cclogMergeUnit->merged_ = true;
            ouputCclog(log);
        }
    }

    // remove merged
    for (it = cclogMergeQueue_.begin(); it != cclogMergeQueue_.end(); )
    {
        boost::shared_ptr<CCLogMerge>& cclogMergeUnit = it->second;
        if (cclogMergeUnit->merged_ == true)
        {
            cclogMergeQueue_.erase(it++);
            continue;
        }
        else
        {
            it++;
        }
    }
}

}

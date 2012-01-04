#include "DriverLogServerController.h"


namespace sf1r
{
using namespace izenelib::driver;
using driver::Keys;


void DriverLogServerController::init()
{
    drum_ = LogServerStorage::get()->getDrum();

    LogServerStorage::get()->getDrumDispatcher().registerOp(
            LogServerStorage::DrumDispatcherImpl::UNIQUE_KEY_CHECK,
            boost::bind(&DriverLogServerController::onUniqueKeyCheck, this, _1, _2, _3));

    LogServerStorage::get()->getDrumDispatcher().registerOp(
            LogServerStorage::DrumDispatcherImpl::DUPLICATE_KEY_CHECK,
            boost::bind(&DriverLogServerController::onDuplicateKeyCheck, this, _1, _2, _3));

    docidDB_ = LogServerStorage::get()->getDocidDB();
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
    std::cout<<request().controller()<<"/"<<request().action()<<std::endl;


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
        return;
    }

    // output TODO
    std::cout<<raw<<std::endl;
}

bool DriverLogServerController::skipProcess(const std::string& collection)
{
    if (driverCollections_.find(collection) == driverCollections_.end())
    {
        std::cout<<"skip "<<collection<<std::endl;
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
void DriverLogServerController::processDocVisit(izenelib::driver::Request& request, const std::string& raw)
{
    //std::cout<<"DocVisitProcessor "<<asString(request[Keys::resource][Keys::DOCID])<<std::endl;
    drum_->Check(asString(request[Keys::resource][Keys::DOCID]), raw);
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
void DriverLogServerController::processRecVisitItem(izenelib::driver::Request& request, const std::string& raw)
{
    //std::cout<<"RecVisitItemProcessor "<<asString(request[Keys::resource][Keys::ITEMID])<<std::endl;
    drum_->Check(asString(request[Keys::resource][Keys::ITEMID]), raw);
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
void DriverLogServerController::processRecPurchaseItem(izenelib::driver::Request& request, const std::string& raw)
{
    //std::cout<<"RecPurchaseItemProcessor ";

    const Value& itemsValue = request[Keys::resource][Keys::items];
    if (nullValue(itemsValue) || itemsValue.type() != izenelib::driver::Value::kArrayType)
    {
        //response().addError("Require an array of items in request[resource][items].");
        return;
    }

    for (std::size_t i = 0; i < itemsValue.size(); ++i)
    {
        const Value& itemValue = itemsValue(i);

        std::string itemIdStr = asString(itemValue[Keys::ITEMID]);
        //std::cout<<itemIdStr<<"  ";
        drum_->Check(itemIdStr, raw);
    }

    //std::cout<<std::endl;
}

void DriverLogServerController::onUniqueKeyCheck(
        const std::string& uuid,
        const std::vector<uint32_t>& docidList,
        const std::string& aux)
{
    std::cout << "onUniqueKeyCheck" << uuid << " --- " << aux << std::endl;
}

void DriverLogServerController::onDuplicateKeyCheck(
        const std::string& uuid,
        const std::vector<uint32_t>& docidList,
        const std::string& aux)
{
    std::cout << "onDuplicateKeyCheck" << uuid << " --- " << aux << std::endl;
}


}

#include "DriverLogProcessor.h"

namespace sf1r
{
using namespace izenelib::driver;
using driver::Keys;

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
void DocVisitProcessor::process(izenelib::driver::Request& request)
{
    // TODO
    std::cout<<"DocVisitProcessor "<<asString(request[Keys::resource][Keys::DOCID])<<std::endl;
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
void RecVisitItemProcessor::process(izenelib::driver::Request& request)
{
    // TODO
    std::cout<<"RecVisitItemProcessor "<<asString(request[Keys::resource][Keys::ITEMID])<<std::endl;
}

/**
 * Process request of recommend/visit_item
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
void RecPurchaseItemProcessor::process(izenelib::driver::Request& request)
{
    // TODO
    std::cout<<"RecPurchaseItemProcessor ";

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
        std::cout<<itemIdStr<<"  ";
    }

    std::cout<<std::endl;
}



ProcessorFactory::ProcessorFactory()
{
    //defaultProcessor_ = new Processor();
    defaultProcessor_ = NULL; // nothing to do

    // Add special processors here
    Processor* processor;
    processor = new DocVisitProcessor();
    specialProcessors_.insert(std::make_pair(ProcessorKey("documents", "visit"), processor));

    processor = new RecVisitItemProcessor();
    specialProcessors_.insert(std::make_pair(ProcessorKey("recommend", "visit_item"), processor));

    processor = new RecPurchaseItemProcessor();
    specialProcessors_.insert(std::make_pair(ProcessorKey("recommend", "purchase_item"), processor));
}

ProcessorFactory::~ProcessorFactory()
{
    if (defaultProcessor_)
        delete defaultProcessor_;

    map_t::iterator it;
    for (it = specialProcessors_.begin(); it != specialProcessors_.end(); it++)
    {
        if (it->second)
            delete it->second;
    }
}

ProcessorPtr ProcessorFactory::getProcessor(
            const std::string& controller,
            const std::string& action) const
{
    ProcessorKey key(controller, action);

    map_t::const_iterator it = specialProcessors_.find(key);
    if (it != specialProcessors_.end())
    {
        return it->second;
    }

    return defaultProcessor_;
}



}

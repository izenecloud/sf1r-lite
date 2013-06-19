#include "QueryIntentHelper.h"

#include <util/driver/Value.h>
#include <util/driver/Request.h>
#include <util/driver/Response.h>
#include <common/Keys.h>

#include <limits>
#include <glog/logging.h>

namespace sf1r
{
using driver::Keys;
using namespace izenelib::driver;

void refineRequest(izenelib::driver::Request& request,
             izenelib::driver::Response& response,
             std::map<QueryIntentCategory, std::list<std::string> >& intents)
{
    izenelib::driver::Value& conditions = request[Keys::conditions];
    izenelib::driver::Value& queryIntents = response["query_intent"];
    std::map<QueryIntentCategory, std::list<std::string> >::iterator array;
    std::list<std::string>::iterator item;
    for (array = intents.begin(); array != intents.end(); array++)
    {
        izenelib::driver::Value& condition = conditions();
        if (array->second.empty())
            continue;
        condition[Keys::property] = array->first.name_;
        condition["operator"] = array->first.op_;
        int operands = array->first.operands_;
        if (-1 == operands)
            operands = std::numeric_limits<int>::max();
        izenelib::driver::Value& values = condition[Keys::value];
        izenelib::driver::Value& queryIntent = queryIntents();
        izenelib::driver::Value& queryIntentValues = queryIntent[array->first.name_];
        int i = 0;
        for (item = array->second.begin(); item != array->second.end(); i++, item++)
        {
            LOG(INFO)<<*item;
            if (i >= operands)
                break;
            values() = *item;
            queryIntentValues() = *item;
            LOG(INFO)<<*item;
        }
    }
}

}

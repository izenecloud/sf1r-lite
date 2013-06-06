#include "QueryIntentHelper.h"

#include <util/driver/Value.h>
#include <common/Keys.h>

#include <glog/logging.h>

namespace sf1r
{
using driver::Keys;
using namespace izenelib::driver;

void rewriteRequest(izenelib::driver::Request& request,
             std::map<QueryIntentType, std::list<std::string> >& intents)
{
    izenelib::driver::Value& conditions = request[Keys::conditions];
    std::map<QueryIntentType, std::list<std::string> >::iterator array;
    std::list<std::string>::iterator item;
    for (array = intents.begin(); array != intents.end(); array++)
    {
        izenelib::driver::Value& condition = conditions();
        if (array->second.empty())
            continue;
        const char* pro = intentToString(array->first);
        if (NULL == pro)
            continue;
        condition[Keys::property] = pro;
        // Other?
        condition["operator"] = "=";
        izenelib::driver::Value& values = condition[Keys::value];
        for (item = array->second.begin(); item != array->second.end(); item++)
        {
            values() = *item;
        }
    }
}

const char* intentToString(QueryIntentType intent)
{
    if (COMMODITY == intent)
        return "";
    else if (MERCHANT == intent)
        return "Source";
    else if (CATEGORY == intent)
        return "Category";
    else
        return NULL;
}

}

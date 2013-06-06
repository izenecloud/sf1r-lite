#include "QueryIntentHelper.h"

#include <util/driver/Value.h>
#include <common/Keys.h>

#include <glog/logging.h>

namespace sf1r
{
using driver::Keys;
using namespace izenelib::driver;

void rewriteRequest(izenelib::driver::Request& request,
             std::list<std::pair<QueryIntentCategory, std::list<std::string> > >& intents)
{
    izenelib::driver::Value& conditions = request[Keys::conditions];
    std::list<std::pair<QueryIntentCategory, std::list<std::string> > >::iterator array;
    std::list<std::string>::iterator item;
    for (array = intents.begin(); array != intents.end(); array++)
    {
        izenelib::driver::Value& condition = conditions();
        if (array->second.empty())
            continue;
        condition[Keys::property] = array->first.property_;
        // Other?
        condition["operator"] = array->first.op_;
        izenelib::driver::Value& values = condition[Keys::value];
        for (item = array->second.begin(); item != array->second.end(); item++)
        {
            values() = *item;
        }
    }
}

}

#include "QueryIntentHelper.h"
#include "Classifier.h"

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
using namespace NQI;

void refineRequest(izenelib::driver::Request& request,
             izenelib::driver::Response& response,
             WMVContainer& wmvs)
{
    if (wmvs.empty())
        return;
    izenelib::driver::Value& conditions = request[Keys::conditions];
    izenelib::driver::Value& queryIntents = response["query_intent"];
    WMVCIterator array;
    WMVIterator item;
    for (array = wmvs.begin(); array != wmvs.end(); array++)
    {
        if (array->second.empty())
            continue;
        izenelib::driver::Value& condition = conditions();
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
            if (i >= operands)
                break;
            values() = item->first;
            LOG(INFO)<<item->first;
            queryIntentValues() = item->first;
        }
    }
}

}

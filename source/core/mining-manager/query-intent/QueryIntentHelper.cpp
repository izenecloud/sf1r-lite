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

static void refineGroupLabels(izenelib::driver::Value& groupLabels,
             izenelib::driver::Value& queryIntents,
             WMVContainer& wmvs)
{
    WMVCIterator array;
    WMVIterator item;
    for (array = wmvs.begin(); array != wmvs.end(); array++)
    {
        if (array->second.empty())
            continue;
        izenelib::driver::Value& groupLabel = groupLabels();
        groupLabel[Keys::property] = array->first.name_;
        int operands = array->first.operands_;
        if (-1 == operands)
            operands = std::numeric_limits<int>::max();
        izenelib::driver::Value& values = groupLabel[Keys::value];
        izenelib::driver::Value& queryIntent = queryIntents();
        izenelib::driver::Value& queryIntentValues = queryIntent[array->first.name_];
        int i = 0;
        for (item = array->second.begin(); item != array->second.end(); i++, item++)
        {
            if (i >= operands)
                break;
            std::string vs = item->first;
            std::size_t pos = 0;
            while (true)
            {
                std::size_t found = vs.find_first_of('>', pos);
                if (std::string::npos == found)
                {
                    std::string valueString = vs.substr(pos);
                    boost::trim(valueString);
                    values() = valueString;
                    LOG(INFO)<<valueString;
                    break;
                }
                else
                {
                    std::string valueString = vs.substr(pos, found - pos);
                    boost::trim(valueString);
                    values() = valueString;
                    LOG(INFO)<<valueString;
                    pos = found + 1;
                }
            }
            queryIntentValues() = item->first;
        }
    }
}

static void refineConditions(izenelib::driver::Value& conditions,
             izenelib::driver::Value& queryIntents,
             WMVContainer& wmvs)
{
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
            queryIntentValues() = item->first;
            LOG(INFO)<<item->first;
        }
    }
}

void refineRequest(izenelib::driver::Request& request,
             izenelib::driver::Response& response,
             WMVContainer& wmvs)
{
    if (wmvs.empty())
        return;
    izenelib::driver::Value& queryIntents = response["query_intent"];
    //if ("suffix" == request[Keys::search][Keys::searching_mode][Keys::mode])
    if (("*" == request[Keys::search][Keys::keywords]) && 
        ("suffix" != request[Keys::search][Keys::searching_mode][Keys::mode]))
    {
        izenelib::driver::Value& conditions = request[Keys::conditions];
        refineConditions(conditions, queryIntents, wmvs);
    }
    else
    {
        izenelib::driver::Value& groupLabels = request[Keys::search][Keys::group_label];
        refineGroupLabels(groupLabels, queryIntents, wmvs);
    }
}

}

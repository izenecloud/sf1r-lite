#include "ProductQueryIntent.h"
#include "QueryIntentHelper.h"

#include <common/Keys.h>

#include <map>
#include <list>

#include <glog/logging.h>

namespace sf1r 
{
using driver::Keys;
using namespace izenelib::driver;

ProductQueryIntent::ProductQueryIntent(IntentContext* context)
    : QueryIntent(context)
{
    classifiers_.clear();
}

ProductQueryIntent::~ProductQueryIntent()
{
    ContainerIterator it = classifiers_.begin();
    for(; it != classifiers_.end(); it++)
    {
        if(it->second.second)
        {
            delete it->second.second;
            it->second.second = NULL;
        }
    }
    classifiers_.clear();
}

void ProductQueryIntent::process(izenelib::driver::Request& request)
{
    if (classifiers_.empty())
        return;
    
    izenelib::driver::Value& conditions = request[Keys::conditions];
    const izenelib::driver::Value::ArrayType* array = conditions.getPtr<Value::ArrayType>();
    if (array && (0 != array->size()))
    {
        for (std::size_t i = 0; i < array->size(); i++)
        {
            std::string condition = asString((*array)[i][Keys::property]);
            ContainerIterator it = classifiers_.find(condition);
            if (classifiers_.end() != it)
                it->second = make_pair(false, it->second.second);
            else
                it->second = make_pair(true, it->second.second);
        }
    }
    std::string keywords = asString(request[Keys::search][Keys::keywords]);
    std::map<QueryIntentCategory, std::list<std::string> > intents;
    bool ret = false;
    ContainerIterator it = classifiers_.begin();
    for(; it != classifiers_.end(); it++)
    {
        if (it->second.first)
            ret |= it->second.second->classify(intents, keywords);
    }
  
    if (!ret)
        return;
    if (keywords.empty())
        request[Keys::search][Keys::keywords] = "*";
    else
        request[Keys::search][Keys::keywords] = keywords;
    rewriteRequest(request, intents);
}

void ProductQueryIntent::reload()
{
    ContainerIterator it = classifiers_.begin();
    for(; it != classifiers_.end(); it++)
    {
        if(it->second.second)
            it->second.second->reload();
    }
}

}

#include "ProductQueryIntent.h"
#include "QueryIntentHelper.h"

#include <common/Keys.h>
#include <common/QueryNormalizer.h>

#include <boost/unordered_map.hpp>
#include <map>
#include <list>

#include <glog/logging.h>

namespace sf1r 
{
using driver::Keys;
using namespace izenelib::driver;
using namespace NQI;

ProductQueryIntent::ProductQueryIntent(IntentContext* context)
    : QueryIntent(context)
{
    for (std::size_t i = 0; i < classifiers_.size(); i++)
    {
        classifiers_[i].clear();
    }
}

ProductQueryIntent::~ProductQueryIntent()
{
    for (std::size_t i = 0; i < classifiers_.size(); i++)
    {
        ContainerIterator it = classifiers_[i].begin();
        for(; it != classifiers_[i].end(); it++)
        {
            if(*it)
            {
                delete *it;
                *it = NULL;
            }
        }
    }
    classifiers_.clear();
}

void ProductQueryIntent::process(izenelib::driver::Request& request, izenelib::driver::Response& response)
{
    if (classifiers_.empty())
        return;
    std::string keywords = asString(request[Keys::search][Keys::keywords]);
    if (keywords.empty() || "*" == keywords)
        return;
    if (keywords.size() > 60)
        return;
    //std::string mode = asString(request[Keys::search][Keys::searching_mode][Keys::mode]);
    //boost::to_lower(mode);
    //if (mode == "suffix")
    //    return;
    std::string normalizedQuery;
    QueryNormalizer::get()->normalize(keywords, normalizedQuery);
    LOG(INFO)<<normalizedQuery; 
    
    bool intentSpecified = false;
    boost::unordered_map<std::string, bool> bitmap;
    izenelib::driver::Value& enables = request["query_intent"];
    const izenelib::driver::Value::ArrayType* array = enables.getPtr<Value::ArrayType>();
    if (array && (0 != array->size()))
    {
        intentSpecified = true;
        for (std::size_t i = 0; i < array->size(); i++)
        {
            std::string property = asString((*array)[i][Keys::property]);
            bitmap.insert(make_pair(property, true));
        }
    }
    
    boost::unordered_map<std::string, std::list<std::string> > scs;
    izenelib::driver::Value& conditions = request[Keys::conditions];
    array = conditions.getPtr<Value::ArrayType>();
    if (array && (0 != array->size()))
    {
        for (std::size_t i = 0; i < array->size(); i++)
        {
            std::string property = asString((*array)[i][Keys::property]);
            const izenelib::driver::Value::ArrayType* values = (*array)[i][Keys::value].getPtr<Value::ArrayType>();
            std::list<std::string> cs; 
            for (std::size_t vi = 0; vi < values->size(); vi++)
            {
                cs.push_back(asString((*values)[vi]));
            }
            scs.insert(make_pair(property, cs));
        }
    }
    
    
    WMVContainer wmvs;
    bool ret = false;
    for (std::size_t priority = 0; priority < classifiers_.size(); priority++)
    {
        ContainerIterator it = classifiers_[priority].begin();
        for(; it != classifiers_[priority].end(); it++)
        {
            boost::unordered_map<std::string, bool>::iterator bIt = bitmap.find((*it)->name());
            if (intentSpecified) 
            {
                if((bitmap.end() != bIt) && (bIt->second) )
                    ret |= (*it)->classify(wmvs, normalizedQuery);
            }
            else if ( bitmap.end() == bIt || bIt->second)
                ret |= (*it)->classify(wmvs, normalizedQuery);
            
        }
    }
    bitmap.clear();
  
    if (!ret)
        return;
    combineWMVS(wmvs, scs, normalizedQuery);
    request[Keys::search][Keys::keywords] = normalizedQuery;
    
    boost::trim(normalizedQuery);
    if (normalizedQuery.empty() )
        request[Keys::search][Keys::keywords] = "*";
    refineRequest(request, response, wmvs);
    wmvs.clear();
}

void ProductQueryIntent::reloadLexicon()
{
    for (std::size_t i = 0; i < classifiers_.size(); i++)
    {
        ContainerIterator it = classifiers_[i].begin();
        for(; it != classifiers_[i].end(); it++)
        {
            if(*it)
            {
                (*it)->reloadLexicon();
            }
        }
    }
}

}

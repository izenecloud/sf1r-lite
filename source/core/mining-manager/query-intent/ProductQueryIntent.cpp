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

ProductQueryIntent::ProductQueryIntent()
{
    classifiers_.clear();
}

ProductQueryIntent::~ProductQueryIntent()
{
    std::list<Classifier*>::iterator it = classifiers_.begin();
    for(; it != classifiers_.end(); it++)
    {
        if(*it)
        {
            delete *it;
            *it = NULL;
        }
    }
    classifiers_.clear();
}

void ProductQueryIntent::process(izenelib::driver::Request& request)
{
    if (classifiers_.empty())
        return;
    
    std::string keywords = asString(request[Keys::search][Keys::keywords]);
    std::map<QueryIntentCategory, std::list<std::string> > intents;
    bool ret = false;
    std::list<Classifier*>::iterator it = classifiers_.begin();
    for(; it != classifiers_.end(); it++)
    {
       ret |= (*it)->classify(intents, keywords);
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
    std::list<Classifier*>::iterator it = classifiers_.begin();
    for(; it != classifiers_.end(); it++)
    {
        if(*it)
            (*it)->reload();
    }
}

}

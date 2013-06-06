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

void ProductQueryIntent::process(izenelib::driver::Request& request)
{
    if (NULL == classifier_)
        return;
    
    std::string keywords = asString(request[Keys::search][Keys::keywords]);
    std::list<std::pair<QueryIntentCategory, std::list<std::string> > > intents;
    int ret = classifier_->classify(intents, keywords);
  
    if (-1 == ret)
        return;
    if (keywords.empty())
        request[Keys::search][Keys::keywords] = "*";
    else
        request[Keys::search][Keys::keywords] = keywords;
    rewriteRequest(request, intents);
}

}

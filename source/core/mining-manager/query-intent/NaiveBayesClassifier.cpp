#include "NaiveBayesClassifier.h"
#include <glog/logging.h>

namespace sf1r
{

bool NaiveBayesClassifier::classify(std::map<QueryIntentCategory, std::list<std::string> >& intents, std::string& query)
{
    //The classifier is just for function test, so arbitrarily return for performance.
    return false; 
    /*std::string source = "女鞋";
    QueryIntentCategory intent;
    intent.name_ = "CATEGORY";
    size_t begin = query.find(source); 
    if (std::string::npos == begin)
        return -1;
    if (-1 == config_->getQueryIntentCategory(intent))
        return -1;
    size_t end = begin + source.size();
    std::list<std::string> list;
    list.push_back(source);
    intents.push_back(make_pair(intent, list));
    query.erase(begin, end);
    return 0;*/
}

}

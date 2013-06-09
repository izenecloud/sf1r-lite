#include "NaiveBayesClassifier.h"
#include <glog/logging.h>

namespace sf1r
{

int NaiveBayesClassifier::classify(std::map<QueryIntentType, std::list<std::string> >& intents, std::string& query)
{
    //The classifier is just for function test, so arbitrarily return for performance.
    return -1; 
    std::string source = "女鞋";
    size_t begin = query.find(source); 
    if (std::string::npos == begin)
        return -1;
    size_t end = begin + source.size();
    std::list<std::string> list;
    list.push_back(source);
    intents[CATEGORY] = list;
    query.erase(begin, end);
    return 0;
}

}

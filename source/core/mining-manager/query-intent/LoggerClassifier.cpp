#include "LoggerClassifier.h"
#include <util/ustring/UString.h>
#include <common/QueryNormalizer.h>
#include <vector>
#include <list>

#include <glog/logging.h>

namespace sf1r
{

using izenelib::util::UString;

const char* LoggerClassifier::type_ = "logger";

bool LoggerClassifier::classify(std::map<QueryIntentCategory, std::list<std::string> >& intents, std::string& query)
{
    if (query.empty())
        return false;
    QueryIntentCategory iCategory;
    iCategory.name_ = context_->name_;
    QIIterator it = context_->config_->find(iCategory);
    if (context_->config_->end() == it)
        return false;
    std::vector<std::vector<std::string> > pathVec;
    std::vector<int> freqVec;
    std::string normalizedQuery;
    QueryNormalizer::get()->normalize(query, normalizedQuery);
    if (!(context_->miningManager_->getFreqGroupLabel(normalizedQuery, context_->name_, 1, pathVec, freqVec)))
        return false;

    if (pathVec.empty())
        return false;
    std::list<std::string> intent;
    std::string category = "";
    for (size_t i = 0; i < pathVec[0].size(); i++)
    {
        category += pathVec[0][i];
        if ( i < pathVec[0].size() - 1)
            category +=">";
    }
    intent.push_back(category);
    intents[*it] = intent;
    return true;
}
}

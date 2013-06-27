#include "LoggerClassifier.h"
#include <util/ustring/UString.h>
#include <vector>
#include <list>

#include <glog/logging.h>

namespace sf1r
{

using izenelib::util::UString;
using namespace NQI;

const char* LoggerClassifier::type_ = "logger";

bool LoggerClassifier::classify(WMVContainer& wmvs, std::string& query)
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
    if (!(context_->miningManager_->getFreqGroupLabel(query, context_->name_, 1, pathVec, freqVec)))
        return false;

    if (pathVec.empty())
        return false;
    std::string category = "";
    for (size_t i = 0; i < pathVec[0].size(); i++)
    {
        category += pathVec[0][i];
        if ( i < pathVec[0].size() - 1)
            category +=">";
    }
    wmvs[*it].push_back(make_pair(category, 1));
    return true;
}
}

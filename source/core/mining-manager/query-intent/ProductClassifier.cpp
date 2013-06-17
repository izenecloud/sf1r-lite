#include "ProductClassifier.h"
#include <util/ustring/UString.h>
#include <vector>
#include <list>

#include <glog/logging.h>

namespace sf1r
{

using izenelib::util::UString;

const char* ProductClassifier::type_ = "product";

bool ProductClassifier::classify(std::map<QueryIntentCategory, std::list<std::string> >& intents, std::string& query)
{
    LOG(INFO)<<"ProductClassifier:"<<priority();
    if (query.empty())
        return false;
    QueryIntentCategory iCategory;
    iCategory.name_ = context_->name_;
    QIIterator it = context_->config_->find(iCategory);
    if (context_->config_->end() == it)
        return false;
    std::vector<std::vector<std::string> > pathVec;
    context_->miningManager_->GetProductCategory(query, 1, pathVec);

    if (pathVec.empty())
        return false;
    std::list<std::string> intent;
    std::string category = "";
    for (size_t i = 0; i < pathVec[0].size(); i++)
    {
        //LOG(INFO)<<i;
        category += pathVec[0][i];
        if ( i < pathVec[0].size() - 1)
            category +=">";
    }
    intent.push_back(category);
    intents[*it] = intent;
    return true;
}
}

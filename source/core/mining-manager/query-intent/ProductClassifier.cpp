#include "ProductClassifier.h"
#include <util/ustring/UString.h>
#include <vector>
#include <list>

#include <glog/logging.h>

namespace sf1r
{

using izenelib::util::UString;
using namespace NQI;
const char* ProductClassifier::type_ = "product";

bool ProductClassifier::classify(WMVContainer& wmvs, std::string& query)
{
    if (query.empty())
        return false;
    std::vector<std::vector<std::string> > pathVec;
    context_->miningManager_->GetProductCategory(query, 1, pathVec);

    if (pathVec.empty())
        return false;
    std::string category = "";
    for (size_t i = 0; i < pathVec[0].size(); i++)
    {
        category += pathVec[0][i];
        if ( i < pathVec[0].size() - 1)
            category +=">";
    }
    wmvs[*keyPtr_].push_back(make_pair(category, 1));
    return true;
}
}

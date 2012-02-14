/**
 * @file core/query-manager/QueryIdentity.cpp
 * @author Ian Yang
 * @date Created <2009-09-29 17:20:50>
 * @date Updated <2009-09-30 09:58:44>
 */
#include "QueryIdentity.h"

#include <algorithm>

namespace sf1r {

void makeQueryIdentity(
        QueryIdentity& identity,
        const KeywordSearchActionItem& item,
        int distActionType,
        int start
)
{
    identity.query = item.env_.queryString_;
    identity.expandedQueryString = item.env_.expandedQueryString_;
    identity.userId = item.env_.userID_;
    identity.start = start;
    identity.searchingMode = item.searchingMode_;
    if (item.searchingMode_ != SearchingMode::KNN)
    {
        identity.rankingType = item.rankingType_;
        identity.laInfo = item.languageAnalyzerInfo_;
        identity.properties = item.searchPropertyList_;
        identity.sortInfo = item.sortPriorityList_;
        identity.filterInfo = item.filteringList_;
        identity.groupParam = item.groupParam_;
        identity.rangeProperty = item.rangePropertyName_;
        identity.strExp = item.strExp_;
        identity.paramConstValueMap = item.paramConstValueMap_;
        identity.paramPropertyValueMap = item.paramPropertyValueMap_;
        identity.distActionType = distActionType;
        std::sort(identity.properties.begin(),
                identity.properties.end());
    }
}

} // namespace sf1r

#include "GroupLabelPreProcessor.h"
#include <bundles/mining/MiningSearchService.h>

#include <glog/logging.h>

namespace sf1r
{

void GroupLabelPreProcessor::process(
    KeywordSearchActionItem& actionItem,
    KeywordSearchResult& searchResult)
{
    faceted::GroupParam& groupParam = actionItem.groupParam_;
    typedef faceted::GroupParam::AutoSelectLimitMap LimitMap;
    LimitMap& limitMap = groupParam.autoSelectLimits_;

    if (limitMap.empty())
        return;

    const std::string& query = actionItem.env_.normalizedQueryString_;
    typedef faceted::GroupParam::GroupLabelMap GroupLabelMap;
    GroupLabelMap& totalLabels = groupParam.groupLabels_;
    GroupLabelMap& autoSelectLabels = searchResult.autoSelectGroupLabels_;

    for (LimitMap::const_iterator limitIt = limitMap.begin();
         limitIt != limitMap.end(); ++limitIt)
    {
        pushTopLabels_(totalLabels, autoSelectLabels,
                       query, limitIt->first, limitIt->second);
    }
}

void GroupLabelPreProcessor::pushTopLabels_(
    faceted::GroupParam::GroupLabelMap& totalLabels,
    faceted::GroupParam::GroupLabelMap& autoSelectLabels,
    const std::string& query,
    const std::string& propName,
    int limit)
{
    typedef faceted::GroupParam::GroupPathVec GroupPathVec;
    GroupPathVec pathVec;
    std::vector<int> freqVec;

    if (! miningSearchService_->getFreqGroupLabel(
            query, propName, limit, pathVec, freqVec))
    {
        LOG(ERROR) << "failed to get frequent label for group property: "
                   << propName << ", query: " << query;
    }

    if (pathVec.empty())
    {
        //if(limit <3) limit = 3;
        miningSearchService_->GetProductCategory(query, limit, pathVec);
    }

    for (GroupPathVec::const_iterator pathIt = pathVec.begin();
         pathIt != pathVec.end(); ++pathIt)
    {
        totalLabels[propName].push_back(*pathIt);
    }
    autoSelectLabels[propName].swap(pathVec);
}

} // namespace sf1r

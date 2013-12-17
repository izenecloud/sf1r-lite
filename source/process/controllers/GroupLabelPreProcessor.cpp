#include "GroupLabelPreProcessor.h"
#include <bundles/mining/MiningSearchService.h>

#include <glog/logging.h>

using namespace faceted;

namespace sf1r
{

void GroupLabelPreProcessor::process(
    KeywordSearchActionItem& actionItem,
    KeywordSearchResult& searchResult)
{
    GroupParam& groupParam = actionItem.groupParam_;
    GroupParam::AutoSelectLimitMap& limitMap = groupParam.autoSelectLimits_;

    if (limitMap.empty())
        return;

    const std::string& query = actionItem.env_.normalizedQueryString_;
    GroupParam::GroupLabelMap& totalLabels = groupParam.groupLabels_;
    GroupParam::GroupLabelScoreMap& autoSelectLabels = searchResult.autoSelectGroupLabels_;

    for (GroupParam::AutoSelectLimitMap::const_iterator limitIt = limitMap.begin();
         limitIt != limitMap.end(); ++limitIt)
    {
        pushTopLabels_(totalLabels, autoSelectLabels,
                       query, limitIt->first, limitIt->second);
    }
}

void GroupLabelPreProcessor::pushTopLabels_(
    faceted::GroupParam::GroupLabelMap& totalLabels,
    faceted::GroupParam::GroupLabelScoreMap& autoSelectLabels,
    const std::string& query,
    const std::string& propName,
    int limit)
{
    GroupParam::GroupPathVec pathVec;
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

    GroupParam::GroupPathScoreVec pathScoreVec;
    pathScoreVec.reserve(pathVec.size());
    for (GroupParam::GroupPathVec::const_iterator pathIt = pathVec.begin();
         pathIt != pathVec.end(); ++pathIt)
    {
        totalLabels[propName].push_back(*pathIt);
        pathScoreVec.push_back(std::make_pair(*pathIt, GroupPathScoreInfo()));
    }
    autoSelectLabels[propName].swap(pathScoreVec);
}

} // namespace sf1r

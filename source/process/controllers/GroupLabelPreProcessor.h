/**
 * @file GroupLabelPreProcessor.h
 * @brief preprocess the search request to auto select top labels
 * @author Jun Jiang
 * @date Created 2012-10-22
 */

#ifndef SF1R_GROUP_LABEL_PRE_PROCESSOR_H
#define SF1R_GROUP_LABEL_PRE_PROCESSOR_H

#include "SearchRequestPreProcessor.h"

namespace sf1r
{
class MiningSearchService;

class GroupLabelPreProcessor : public SearchRequestPreProcessor
{
public:
    GroupLabelPreProcessor(MiningSearchService* miningSearchService)
        : miningSearchService_(miningSearchService)
    {}

    virtual void process(
        KeywordSearchActionItem& actionItem,
        KeywordSearchResult& searchResult);

private:
    void pushTopLabels_(
        faceted::GroupParam::GroupLabelMap& totalLabels,
        faceted::GroupParam::GroupLabelMap& autoSelectLabels,
        const std::string& query,
        const std::string& propName,
        int limit);

    MiningSearchService* miningSearchService_;
};

} // namespace sf1r

#endif // SF1R_GROUP_LABEL_PRE_PROCESSOR_H

/**
 * @file RecommendParam.h
 * @author Jun Jiang
 * @date 2011-10-14
 */

#ifndef RECOMMEND_PARAM_H
#define RECOMMEND_PARAM_H

#include "RecTypes.h"
#include "ItemCondition.h"

#include <string>
#include <vector>

namespace sf1r
{

struct RecommendParam
{
    RecommendParam();

    bool check(std::string& errorMsg) const;

    RecommendType type;

    int limit;
    std::string sessionIdStr;
    ItemCondition condition;

    std::string userIdStr;
    std::vector<std::string> inputItems;
    std::vector<std::string> includeItems;
    std::vector<std::string> excludeItems;

    std::vector<std::string> selectRecommendProps;
    std::vector<std::string> selectReasonProps;

    std::vector<itemid_t> inputItemIds;
    std::vector<itemid_t> includeItemIds;
    std::vector<itemid_t> excludeItemIds;
};

} // namespace sf1r

#endif // RECOMMEND_PARAM_H

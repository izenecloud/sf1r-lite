/**
 * @file RecommendParam.h
 * @author Jun Jiang
 * @date 2011-10-14
 */

#ifndef RECOMMEND_PARAM_H
#define RECOMMEND_PARAM_H

#include "RecTypes.h"
#include "ItemCondition.h"
#include "RecommendInputParam.h"

#include <string>
#include <vector>

namespace sf1r
{
class QueryBuilder;

struct RecommendParam
{
    RecommendParam();

    bool check(std::string& errorMsg) const;

    RecommendType type;

    std::string query;
    int queryClickFreq;

    std::string userIdStr;
    std::string sessionIdStr;
    std::vector<std::string> inputItems;
    std::vector<std::string> includeItems;
    std::vector<std::string> excludeItems;

    std::vector<std::string> selectRecommendProps;
    std::vector<std::string> selectReasonProps;

    std::vector<itemid_t> includeItemIds;
    std::vector<itemid_t> excludeItemIds;

    ItemCondition condition;
    RecommendInputParam inputParam;

    void enableItemCondition(
        ItemManager* itemManager,
        QueryBuilder* queryBuilder
    );
};

} // namespace sf1r

#endif // RECOMMEND_PARAM_H

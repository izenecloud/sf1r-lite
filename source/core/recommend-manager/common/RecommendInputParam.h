/**
 * @file RecommendInputParam.h
 * @author Jun Jiang
 * @date 2012-03-27
 */

#ifndef RECOMMEND_INPUT_PARAM_H
#define RECOMMEND_INPUT_PARAM_H

#include "RecTypes.h"
#include "ItemFilter.h"
#include <3rdparty/msgpack/msgpack.hpp>

#include <vector>

namespace sf1r
{

struct RecommendInputParam
{
    RecommendInputParam() : limit(0) {}

    RecommendInputParam(const ItemCondition* itemCondition)
        : limit(0), itemFilter(itemCondition)
    {}

    int limit;
    ItemFilter itemFilter;

    std::vector<itemid_t> inputItemIds;
    ItemCFManager::ItemWeightMap itemWeightMap;

    MSGPACK_DEFINE(limit, itemFilter, inputItemIds, itemWeightMap);
};

} // namespace sf1r

#endif // RECOMMEND_INPUT_PARAM_H

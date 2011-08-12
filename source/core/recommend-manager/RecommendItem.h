/**
 * @file RecommendItem.h
 * @author Jun Jiang
 * @date 2011-08-09
 */

#ifndef RECOMMEND_ITEM_H
#define RECOMMEND_ITEM_H

#include "RecTypes.h"
#include "Item.h"
#include <idmlib/resys/RecommendItem.h>

#include <string>
#include <vector>

namespace sf1r
{

struct ReasonItem
{
    itemid_t itemId_;
    Item item_;
    std::string event_;

    ReasonItem()
        : itemId_(0)
    {}
};

struct RecommendItem
{
    itemid_t itemId_;
    Item item_;
    double weight_;
    std::vector<ReasonItem> reasonItems_;

    RecommendItem()
        : itemId_(0)
        , weight_(0)
    {}

    RecommendItem(const idmlib::recommender::RecommendItem& recItem)
        : itemId_(recItem.itemId_)
        , weight_(recItem.weight_)
        , reasonItems_(recItem.reasonItemIds_.size())
    {
        for (unsigned int i = 0; i < reasonItems_.size(); ++i)
        {
            reasonItems_[i].itemId_ = recItem.reasonItemIds_[i];
        }
    }
};

} // namespace sf1r

#endif // RECOMMEND_ITEM_H

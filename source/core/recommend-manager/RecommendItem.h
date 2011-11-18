/**
 * @file RecommendItem.h
 * @author Jun Jiang
 * @date 2011-08-09
 */

#ifndef RECOMMEND_ITEM_H
#define RECOMMEND_ITEM_H

#include "RecTypes.h"
#include <document-manager/Document.h>
#include <idmlib/resys/RecommendItem.h>

#include <string>
#include <vector>

namespace sf1r
{

struct ReasonItem
{
    Document item_;
    std::string event_;
    std::string value_;
};

struct RecommendItem
{
    Document item_;
    double weight_;
    std::vector<ReasonItem> reasonItems_;

    RecommendItem()
        : weight_(0)
    {}

    RecommendItem(const idmlib::recommender::RecommendItem& recItem)
        : item_(recItem.itemId_)
        , weight_(recItem.weight_)
        , reasonItems_(recItem.reasonItemIds_.size())
    {
        for (unsigned int i = 0; i < reasonItems_.size(); ++i)
        {
            reasonItems_[i].item_.setId(recItem.reasonItemIds_[i]);
        }
    }
};

} // namespace sf1r

#endif // RECOMMEND_ITEM_H

/**
 * @file ItemFilter.h
 * @author Jun Jiang
 * @date 2011-05-03
 */

#ifndef ITEM_FILTER_H
#define ITEM_FILTER_H

#include <idmlib/resys/ItemRescorer.h>
#include "RecTypes.h"
#include "ItemManager.h"

#include <vector>

namespace sf1r
{

class ItemFilter : public idmlib::recommender::ItemRescorer
{
public:
    ItemFilter(
        ItemManager* itemManager,
        const std::vector<itemid_t>& filterList
    )
        : itemManager_(itemManager)
        , filterSet_(filterList.begin(), filterList.end())
    {
    }

    float rescore(itemid_t itemId, float originalScore)
    {
        return 0;
    }

    /**
     * @return true to filter @p itemId, false to assume @p itemId as recommendation candidate.
     */
    bool isFiltered(itemid_t itemId)
    {
        return filterSet_.find(itemId) != filterSet_.end() || !itemManager_->hasItem(itemId);
    }

private:
    ItemManager* itemManager_;
    std::set<itemid_t> filterSet_;
};

} // namespace sf1r

#endif // ITEM_FILTER_H

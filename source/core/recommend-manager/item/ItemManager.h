/**
 * @file ItemManager.h
 * @author Jun Jiang
 * @date 2012-02-14
 */

#ifndef ITEM_MANAGER_H
#define ITEM_MANAGER_H

#include "../common/RecTypes.h"

#include <vector>
#include <string>

namespace sf1r
{
class ItemContainer;

class ItemManager
{
public:
    virtual ~ItemManager() {}

    /**
     * @return true for the document of @p itemId exists, false for not exists
     */
    virtual bool hasItem(itemid_t itemId) = 0;

    /**
     * Get property values for each item in @p itemContainer.
     *
     * @param[in] propList property name list
     * @param[out] itemContainer each item stores property values
     *
     * @return true for all items property values are got;
     *         false for failed to get all items property values, after this function returns,
     *         for each item, @c Document::isEmpty() gives whether this item is failed.
     *
     * @pre in @p itemContainer, each @c Document::getId() gives the item id.
     *
     * @note for those properties not appear in @p propList, the implementation could
     *       choose to whether include their property values in @p doc by its own policy.
     */
    virtual bool getItemProps(
        const std::vector<std::string>& propList,
        ItemContainer& itemContainer
    ) = 0;
};

} // namespace sf1r

#endif // ITEM_MANAGER_H

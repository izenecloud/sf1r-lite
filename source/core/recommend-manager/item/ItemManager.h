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
class Document;

class ItemManager
{
public:
    virtual ~ItemManager() {}

    /**
     * Get an item by @p itemId.
     * @param[in] itemId item id
     * @param[in] propList property name list, it specifies which property values
     *                     need to be included in @p doc
     * @param[out] doc the item result
     * @return true for the document of @p itemId exists, false for not exists
     * @note for those properties not appear in @p propList, the implementation could
     *       choose to whether include their property values in @p doc by its own policy
     */
    virtual bool getItem(
        itemid_t itemId,
        const std::vector<std::string>& propList,
        Document& doc
    ) = 0;

    /**
     * @return true for the document of @p itemId exists, false for not exists
     */
    virtual bool hasItem(itemid_t itemId) const = 0;
};

} // namespace sf1r

#endif // ITEM_MANAGER_H

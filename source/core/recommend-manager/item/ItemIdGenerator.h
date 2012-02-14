/**
 * @file ItemIdGenerator.h
 * @author Jun Jiang
 * @date 2012-02-14
 */

#ifndef ITEM_ID_GENERATOR_H
#define ITEM_ID_GENERATOR_H

#include "../common/RecTypes.h"

namespace sf1r
{

class ItemIdGenerator
{
public:
    virtual ~ItemIdGenerator() {}

    /**
     * Convert from @p strId to @p itemId.
     * @param strId external string id supplied by user 
     * @param itemId internal item id as conversion result
     * @return true for success, false for failure
     */
    virtual bool strIdToItemId(const std::string& strId, itemid_t& itemId) = 0;

    /**
     * Convert from @p itemId to @p strId.
     * @param itemId internal item id
     * @param strId external string id as conversion result
     * @return true for success, false for failure
     */
    virtual bool itemIdToStrId(itemid_t itemId, std::string& strId) = 0;

    /**
     * @return current max item id, 0 for no item id exists
     */
    virtual itemid_t maxItemId() const = 0;
};

} // namespace sf1r

#endif // ITEM_ID_GENERATOR_H

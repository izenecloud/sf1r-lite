/**
 * @file ItemIdGenerator.h
 * @author Jun Jiang
 * @date 2011-04-19
 */

#ifndef ITEM_ID_GENERATOR_H
#define ITEM_ID_GENERATOR_H

#include "RecTypes.h"
#include <ir/id_manager/IDManager.h>

namespace sf1r
{

class ItemIdGenerator
{
public:
    typedef izenelib::ir::idmanager::IDManager IDManager;

    ItemIdGenerator(IDManager* idManager);

    /**
     * Convert from @p strId to @p itemId.
     * @param strId item string id supplied by user 
     * @param itemId conversion result
     * @return true for success, that is, @p strId was inserted before,
     *         false for failure, that is, @p strId does not exist
     */
    bool getItemIdByStrId(const std::string& strId, itemid_t& itemId);

private:
    IDManager* idManager_;
};

} // namespace sf1r

#endif // ITEM_ID_GENERATOR_H

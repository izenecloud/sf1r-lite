/**
 * @file LocalItemIdGenerator.h
 * @author Jun Jiang
 * @date 2011-04-19
 */

#ifndef LOCAL_ITEM_ID_GENERATOR_H
#define LOCAL_ITEM_ID_GENERATOR_H

#include "ItemIdGenerator.h"
#include <ir/id_manager/IDManager.h>

namespace sf1r
{

class LocalItemIdGenerator : public ItemIdGenerator
{
public:
    typedef izenelib::ir::idmanager::IDManager IDManagerType;

    LocalItemIdGenerator(IDManagerType& idManager);

    virtual bool strIdToItemId(const std::string& strId, itemid_t& itemId);

    virtual bool itemIdToStrId(itemid_t itemId, std::string& strId);

    virtual itemid_t maxItemId() const;

private:
    IDManagerType& idManager_;
};

} // namespace sf1r

#endif // LOCAL_ITEM_ID_GENERATOR_H

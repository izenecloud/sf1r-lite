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
class DocumentManager;

class LocalItemIdGenerator : public ItemIdGenerator
{
public:
    typedef izenelib::ir::idmanager::IDManager IDManager;

    LocalItemIdGenerator(IDManager& idManager, DocumentManager& docManager);

    virtual bool strIdToItemId(const std::string& strId, itemid_t& itemId);

    virtual bool itemIdToStrId(itemid_t itemId, std::string& strId);

    virtual itemid_t maxItemId() const;

private:
    IDManager& idManager_;
    DocumentManager& docManager_;
};

} // namespace sf1r

#endif // LOCAL_ITEM_ID_GENERATOR_H

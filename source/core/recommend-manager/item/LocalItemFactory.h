/**
 * @file LocalItemFactory.h
 * @brief create item related instances with local storage
 * @author Jun Jiang
 * @date 2012-02-14
 */

#ifndef LOCAL_ITEM_FACTORY_H
#define LOCAL_ITEM_FACTORY_H

#include "ItemFactory.h"
#include <ir/id_manager/IDManager.h>

namespace sf1r
{
class DocumentManager;
class ItemIdGenerator;
class ItemManager;

class LocalItemFactory : public ItemFactory
{
public:
    typedef izenelib::ir::idmanager::IDManager IDManager;

    LocalItemFactory(IDManager* idManager, DocumentManager* docManager);

    virtual ItemIdGenerator* createItemIdGenerator();

    virtual ItemManager* createItemManager();

private:
    IDManager* idManager_;
    DocumentManager* docManager_;
};

} // namespace sf1r

#endif // LOCAL_ITEM_FACTORY_H

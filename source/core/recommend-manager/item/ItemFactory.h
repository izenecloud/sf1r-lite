/**
 * @file ItemFactory.h
 * @brief create item related instances, such as ItemIdGenerator, ItemManager.
 * @author Jun Jiang
 * @date 2012-02-14
 */

#ifndef ITEM_FACTORY_H
#define ITEM_FACTORY_H

#include <ir/id_manager/IDManager.h>

namespace sf1r
{
class DocumentManager;
class ItemIdGenerator;
class ItemManager;

class ItemFactory
{
public:
    typedef izenelib::ir::idmanager::IDManager IDManager;

    ItemFactory(IDManager* idManager, DocumentManager* docManager);

    ItemIdGenerator* createItemIdGenerator();
    ItemManager* createItemManager();

private:
    IDManager* idManager_;
    DocumentManager* docManager_;
};

} // namespace sf1r

#endif // ITEM_FACTORY_H

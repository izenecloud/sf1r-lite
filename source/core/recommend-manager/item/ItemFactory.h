/**
 * @file ItemFactory.h
 * @brief create item related instances, such as ItemIdGenerator, ItemManager.
 * @author Jun Jiang
 * @date 2012-02-14
 */

#ifndef ITEM_FACTORY_H
#define ITEM_FACTORY_H

namespace sf1r
{
class ItemIdGenerator;
class ItemManager;

class ItemFactory
{
public:
    virtual ~ItemFactory() {}

    virtual ItemIdGenerator* createItemIdGenerator() = 0;

    virtual ItemManager* createItemManager() = 0;
};

} // namespace sf1r

#endif // ITEM_FACTORY_H

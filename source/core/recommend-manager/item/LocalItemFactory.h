/**
 * @file LocalItemFactory.h
 * @brief create item related instances with local storage
 * @author Jun Jiang
 * @date 2012-02-14
 */

#ifndef LOCAL_ITEM_FACTORY_H
#define LOCAL_ITEM_FACTORY_H

#include "ItemFactory.h"

namespace sf1r
{
class IndexSearchService;

class LocalItemFactory : public ItemFactory
{
public:
    LocalItemFactory(IndexSearchService& indexSearchService);

    virtual ItemIdGenerator* createItemIdGenerator();

    virtual ItemManager* createItemManager();

private:
    IndexSearchService& indexSearchService_;
};

} // namespace sf1r

#endif // LOCAL_ITEM_FACTORY_H

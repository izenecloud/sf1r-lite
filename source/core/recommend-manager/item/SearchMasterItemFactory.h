/**
 * @file SearchMasterItemFactory.h
 * @brief create item related instances on search master node
 * @author Jun Jiang
 * @date 2012-02-22
 */

#ifndef SEARCH_MASTER_ITEM_FACTORY_H
#define SEARCH_MASTER_ITEM_FACTORY_H

#include "ItemFactory.h"
#include <string>

namespace sf1r
{
class IndexSearchService;

class SearchMasterItemFactory : public ItemFactory
{
public:
    SearchMasterItemFactory(
        const std::string& collection,
        IndexSearchService& indexSearchService
    );

    virtual ItemIdGenerator* createItemIdGenerator();

    virtual ItemManager* createItemManager();

private:
    const std::string collection_;
    IndexSearchService& indexSearchService_;
};

} // namespace sf1r

#endif // SEARCH_MASTER_ITEM_FACTORY_H

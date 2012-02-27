/**
 * @file RemoteItemFactory.h
 * @brief create item related instances to get doc on remote search master
 * @author Jun Jiang
 * @date 2012-02-27
 */

#ifndef REMOTE_ITEM_FACTORY_H
#define REMOTE_ITEM_FACTORY_H

#include "ItemFactory.h"
#include <string>

namespace sf1r
{

class RemoteItemFactory : public ItemFactory
{
public:
    RemoteItemFactory(const std::string& collection);

    virtual ItemIdGenerator* createItemIdGenerator();

    virtual ItemManager* createItemManager();

private:
    const std::string collection_;
};

} // namespace sf1r

#endif // REMOTE_ITEM_FACTORY_H

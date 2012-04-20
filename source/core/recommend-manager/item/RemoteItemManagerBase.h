/**
 * @file RemoteItemManagerBase.h
 * @author Jun Jiang
 * @date 2011-04-22
 */

#ifndef REMOTE_ITEM_MANAGER_BASE_H
#define REMOTE_ITEM_MANAGER_BASE_H

#include "ItemManager.h"

#include <string>
#include <vector>
#include <boost/scoped_ptr.hpp>

namespace sf1r
{
class ItemIdGenerator;
class GetDocumentsByIdsActionItem;
class RawTextResultFromSIA;

class RemoteItemManagerBase : public ItemManager
{
public:
    RemoteItemManagerBase(
        const std::string& collection,
        ItemIdGenerator* itemIdGenerator
    );

    virtual bool hasItem(itemid_t itemId);

    virtual bool getItemProps(
        const std::vector<std::string>& propList,
        ItemContainer& itemContainer
    );

protected:
    virtual bool sendRequest_(
        const GetDocumentsByIdsActionItem& request,
        RawTextResultFromSIA& response
    ) = 0;

    bool createRequest_(
        const std::vector<std::string>& propList,
        ItemContainer& itemContainer,
        GetDocumentsByIdsActionItem& request
    );

    bool getItemsFromResponse_(
        const std::vector<std::string>& propList,
        const RawTextResultFromSIA& response,
        ItemContainer& itemContainer
    ) const;

protected:
    const std::string collection_;
    boost::scoped_ptr<ItemIdGenerator> itemIdGenerator_;
};

} // namespace sf1r

#endif // REMOTE_ITEM_MANAGER_BASE_H

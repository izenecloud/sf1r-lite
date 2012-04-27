#include "LocalItemManager.h"
#include "ItemContainer.h"
#include <document-manager/DocumentManager.h>
#include <document-manager/Document.h>

namespace sf1r
{

LocalItemManager::LocalItemManager(DocumentManager& docManager)
    : docManager_(docManager)
{
}

bool LocalItemManager::hasItem(itemid_t itemId)
{
    return itemId <= docManager_.getMaxDocId() &&
           !docManager_.isDeleted(itemId);
}

bool LocalItemManager::getItemProps(
    const std::vector<std::string>& propList,
    ItemContainer& itemContainer
)
{
    bool result = true;
    const std::size_t itemNum = itemContainer.getItemNum();

    for (std::size_t i = 0; i < itemNum; ++i)
    {
        Document& doc = itemContainer.getItem(i);
        doc.clearProperties();

        if (! docManager_.getDocument(doc.getId(), doc))
            result = false;
    }

    return result;
}

} // namespace sf1r

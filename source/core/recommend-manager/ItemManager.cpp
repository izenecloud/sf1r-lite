#include "ItemManager.h"
#include <document-manager/DocumentManager.h>
#include <document-manager/Document.h>

namespace sf1r
{

ItemManager::ItemManager(DocumentManager* docManager)
    : docManager_(docManager)
{
}

bool ItemManager::getItem(itemid_t itemId, Document& doc)
{
    return docManager_->getDocument(itemId, doc);
}

bool ItemManager::hasItem(itemid_t itemId) const
{
    return docManager_->isDeleted(itemId) == false;
}

itemid_t ItemManager::maxItemId() const
{
    return docManager_->getMaxDocId();
}

} // namespace sf1r

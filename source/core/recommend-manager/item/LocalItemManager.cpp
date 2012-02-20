#include "LocalItemManager.h"
#include <document-manager/DocumentManager.h>
#include <document-manager/Document.h>

namespace sf1r
{

LocalItemManager::LocalItemManager(DocumentManager& docManager)
    : docManager_(docManager)
{
}

bool LocalItemManager::getItem(itemid_t itemId, Document& doc)
{
    return docManager_.getDocument(itemId, doc);
}

bool LocalItemManager::hasItem(itemid_t itemId) const
{
    return docManager_.isDeleted(itemId) == false;
}

} // namespace sf1r

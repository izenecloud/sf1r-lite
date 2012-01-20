#include "PurchaseManager.h"
#include <recommend-manager/RecommendMatrix.h>

namespace sf1r
{

bool PurchaseManager::addPurchaseItem(
    const std::string& userId,
    const std::vector<itemid_t>& itemVec,
    RecommendMatrix* matrix
)
{
    ItemIdSet itemIdSet;
    if (! getPurchaseItemSet(userId, itemIdSet))
        return false;

    std::list<itemid_t> oldItems(itemIdSet.begin(), itemIdSet.end());
    std::list<itemid_t> newItems;

    for (std::vector<itemid_t>::const_iterator it = itemVec.begin();
        it != itemVec.end(); ++it)
    {
        if (itemIdSet.insert(*it).second)
        {
            newItems.push_back(*it);
        }
    }

    // not purchased yet
    if (! newItems.empty())
    {
        if (! savePurchaseItem_(userId, itemIdSet, newItems))
            return false;

        if (matrix)
        {
            matrix->update(oldItems, newItems);
        }
    }

    return true;
}

} // namespace sf1r

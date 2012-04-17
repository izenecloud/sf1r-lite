#include "ItemFilter.h"
#include "ItemCondition.h"
#include <document-manager/Document.h>

namespace sf1r
{

ItemFilter::ItemFilter()
    : condition_(NULL)
{
}

bool ItemFilter::isFiltered(itemid_t itemId) const
{
    if (filterSet_.find(itemId) != filterSet_.end())
        return true;

    if (condition_ && !condition_->isMeetCondition(itemId))
        return true;

    return false;
}

} // namespace sf1r

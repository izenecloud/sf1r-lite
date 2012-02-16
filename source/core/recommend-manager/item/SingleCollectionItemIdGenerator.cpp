#include "SingleCollectionItemIdGenerator.h"

#include <boost/filesystem.hpp>
#include <glog/logging.h>

namespace sf1r
{

SingleCollectionItemIdGenerator::SingleCollectionItemIdGenerator(const std::string& dir)
    : idManager_((boost::filesystem::path(dir) / "item").string())
{
}

bool SingleCollectionItemIdGenerator::strIdToItemId(const std::string& strId, itemid_t& itemId)
{
    idManager_.getDocIdByDocName(strId, itemId);
    return true;
}

bool SingleCollectionItemIdGenerator::itemIdToStrId(itemid_t itemId, std::string& strId)
{
    if (idManager_.getDocNameByDocId(itemId, strId))
        return true;

    LOG(WARNING) << "in itemIdToStrId(), item id " << itemId << " does not exist";
    return false;
}

itemid_t SingleCollectionItemIdGenerator::maxItemId() const
{
    return idManager_.getMaxDocId();
}

} // namespace sf1r

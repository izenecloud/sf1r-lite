#include "LocalItemIdGenerator.h"

#include <common/Utilities.h>
#include <glog/logging.h>

namespace
{
const izenelib::util::UString::EncodingType ENCODING_UTF8 = izenelib::util::UString::UTF_8;
}

namespace sf1r
{

LocalItemIdGenerator::LocalItemIdGenerator(IDManagerType& idManager)
    : idManager_(idManager)
{
}

bool LocalItemIdGenerator::strIdToItemId(const std::string& strId, itemid_t& itemId)
{
    if (idManager_.getDocIdByDocName(Utilities::md5ToUint128(strId), itemId, false))
        return true;

    LOG(WARNING) << "in strIdToItemId(), str id " << strId << " has not been inserted before";
    return false;
}

bool LocalItemIdGenerator::itemIdToStrId(itemid_t itemId, std::string& strId)
{
    uint128_t intId;

    if (idManager_.getDocNameByDocId(itemId, intId))
    {
        strId = Utilities::uint128ToMD5(intId);
        return true;
    }

    LOG(WARNING) << "in itemIdToStrId(), failed to convert from item id " << itemId << " to string id";
    return false;
}

itemid_t LocalItemIdGenerator::maxItemId() const
{
    return idManager_.getMaxDocId();
}

} // namespace sf1r

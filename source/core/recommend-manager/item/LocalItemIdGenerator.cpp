#include "LocalItemIdGenerator.h"
#include <util/ustring/UString.h>

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
    izenelib::util::UString ustr(strId, ENCODING_UTF8);

    if (idManager_.getDocIdByDocName(ustr, itemId, false))
        return true;

    LOG(WARNING) << "in strIdToItemId(), str id " << strId << " has not been inserted before";
    return false;
}

bool LocalItemIdGenerator::itemIdToStrId(itemid_t itemId, std::string& strId)
{
    izenelib::util::UString ustr;

    if (idManager_.getDocNameByDocId(itemId, ustr))
    {
        ustr.convertString(strId, ENCODING_UTF8);
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

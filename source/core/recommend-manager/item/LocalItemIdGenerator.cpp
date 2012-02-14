#include "LocalItemIdGenerator.h"

#include <document-manager/DocumentManager.h>
#include <util/ustring/UString.h>

#include <glog/logging.h>

namespace sf1r
{

LocalItemIdGenerator::LocalItemIdGenerator(IDManager& idManager, DocumentManager& docManager)
    : idManager_(idManager)
    , docManager_(docManager)
{
}

bool LocalItemIdGenerator::strIdToItemId(const std::string& strId, itemid_t& itemId)
{
    izenelib::util::UString ustr(strId, UString::UTF_8);

    if (idManager_.getDocIdByDocName(ustr, itemId, false))
        return true;

    LOG(WARNING) << "in LocalItemIdGenerator::strIdToItemId(), str id " << strId << " has not been inserted before";
    return false;
}

bool LocalItemIdGenerator::itemIdToStrId(itemid_t itemId, std::string& strId)
{
    izenelib::util::UString ustr;

    if (idManager_.getDocNameByDocId(itemId, ustr))
    {
        ustr.convertString(strId, UString::UTF_8);
        return true;
    }

    LOG(WARNING) << "in LocalItemIdGenerator::itemIdToStrId(), item id " << itemId << " does not exist";
    return false;
}

itemid_t LocalItemIdGenerator::maxItemId() const
{
    return docManager_.getMaxDocId();
}

} // namespace sf1r

#include "ItemIdGenerator.h"

#include <util/ustring/UString.h>

namespace sf1r
{

ItemIdGenerator::ItemIdGenerator(IDManager* idManager)
    : idManager_(idManager)
{
}

bool ItemIdGenerator::getItemIdByStrId(const std::string& strId, itemid_t& itemId)
{
    izenelib::util::UString ustr(strId, UString::UTF_8);

    return idManager_->getDocIdByDocName(ustr, itemId, false);
}

}

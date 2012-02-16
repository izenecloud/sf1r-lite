/**
 * @file SingleCollectionItemIdGenerator.h
 * @author Jun Jiang
 * @date 2012-02-15
 */

#ifndef SINGLE_COLLECTION_ITEM_ID_GENERATOR_H
#define SINGLE_COLLECTION_ITEM_ID_GENERATOR_H

#include "ItemIdGenerator.h"
#include <ir/id_manager/IDManager.h>

#include <string>

namespace sf1r
{

class SingleCollectionItemIdGenerator : public ItemIdGenerator
{
public:
    SingleCollectionItemIdGenerator(const std::string& dir);

    virtual bool strIdToItemId(const std::string& strId, itemid_t& itemId);

    virtual bool itemIdToStrId(itemid_t itemId, std::string& strId);

    virtual itemid_t maxItemId() const;

private:
    typedef izenelib::ir::idmanager::_IDManager<std::string, itemid_t,
            izenelib::util::NullLock,
            izenelib::ir::idmanager::EmptyWildcardQueryHandler<std::string, itemid_t>,
            izenelib::ir::idmanager::EmptyIDGenerator<std::string, itemid_t>,
            izenelib::ir::idmanager::EmptyIDStorage<std::string, itemid_t>,
            izenelib::ir::idmanager::UniqueIDGenerator<std::string, itemid_t, izenelib::util::ReadWriteLock>,
            izenelib::ir::idmanager::HDBIDStorage<std::string, itemid_t, izenelib::util::ReadWriteLock> > IDManagerType;

    IDManagerType idManager_;
};

} // namespace sf1r

#endif // SINGLE_COLLECTION_ITEM_ID_GENERATOR_H

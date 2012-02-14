/**
 * @file ItemIndex.h
 * @author Yingfeng Zhang
 * @date 2011-05-03
 */

#ifndef ITEM_INDEX_H
#define ITEM_INDEX_H

#include "../common/RecTypes.h"
#include <ir/index_manager/index/IndexReader.h>
#include <ir/index_manager/index/Indexer.h>
#include <ir/index_manager/index/TermIterator.h>
#include <ir/index_manager/index/TermReader.h>
#include <ir/index_manager/index/Term.h>

#include <boost/shared_ptr.hpp>
#include <list>
#include <vector>
#include <string>

namespace sf1r
{

namespace iii = izenelib::ir::indexmanager;
typedef uint32_t ItemIndexDocIDType;
typedef std::vector<uint32_t>::const_iterator DataVectorIterator;

class ItemIndex
{
public:
    ItemIndex(const std::string& path, int64_t memorySize);

    void flush();

    void optimize();

    bool add(ItemIndexDocIDType docId, DataVectorIterator firstIt, DataVectorIterator lastIt);

    bool update(ItemIndexDocIDType docId, DataVectorIterator firstIt, DataVectorIterator lastIt);

    bool del(ItemIndexDocIDType docId);

    bool get(std::list<uint32_t>& itemIds, std::list<ItemIndexDocIDType>& docs);

    uint32_t get(std::vector<uint32_t>& itemIds);

    size_t itemFreq(uint32_t itemId);

    size_t getNumItems();

private:
    inline static uint32_t kStartFieldId()
    {
        return 1;
    }

    std::string property_;

    uint32_t propertyId_;

    boost::shared_ptr<iii::Indexer> indexer_;
};

} // namespace sf1r

#endif // ITEM_INDEX_H


///
/// @file PropIdTable.h
/// @brief store property value ids for each doc
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2012-05-30
///

#ifndef SF1R_PROP_ID_TABLE_H
#define SF1R_PROP_ID_TABLE_H

#include "../faceted-submanager/faceted_types.h"
#include <vector>
#include <boost/static_assert.hpp>

NS_FACETED_BEGIN

template <typename valueid_t, typename index_t>
struct PropIdTable
{
    PropIdTable();

    class PropIdList;

    void getIdList(docid_t docId, PropIdList& propIdList) const;

    void appendIdList(const std::vector<valueid_t>& inputIdList);

    /// key: doc id
    /// value: if the most significant bit is 0, it's just the single value id
    ///        for the doc, otherwise, the following bits give the index in @c multiValueTable_.
    std::vector<index_t> indexTable_;

    /// key: index
    /// value: the count of value ids for the doc,
    ///        the following values are each value id for the doc.
    std::vector<valueid_t> multiValueTable_;

    /// the most significant bit for index type
    static const index_t INDEX_MSB = index_t(1) << (sizeof(index_t)*8 - 1);

    /// the mask to get the following bits for index type
    static const index_t INDEX_MASK = ~INDEX_MSB;

private:
    /// @see the requirement on parameter type size by #indexTable_
    BOOST_STATIC_ASSERT(sizeof(valueid_t) < sizeof(index_t));
};

template <typename valueid_t, typename index_t>
class PropIdTable<valueid_t, index_t>::PropIdList
{
public:
    PropIdList() : size_(0), singleValueId_(0) {}

    std::size_t size() const { return size_; }

    bool empty() const { return size_ == 0; }

    valueid_t operator[](std::size_t i) const
    {
        if (i >= size_)
            return 0;

        return (size_ == 1) ? singleValueId_ : multiValueIter_[i];
    }

    void clear()
    {
        size_ = 0;
        singleValueId_ = 0;
    }

private:
    /// count of value ids
    valueid_t size_;

    /// when there is only one value id
    valueid_t singleValueId_;

    /// when there are multiple value ids, the iterator to the first value id 
    typename std::vector<valueid_t>::const_iterator multiValueIter_;

    friend struct PropIdTable<valueid_t, index_t>;
};

template <typename valueid_t, typename index_t>
PropIdTable<valueid_t, index_t>::PropIdTable()
    : indexTable_(1) // doc id 0 is reserved for an empty doc
{
}

template <typename valueid_t, typename index_t>
void PropIdTable<valueid_t, index_t>::getIdList(docid_t docId, PropIdList& propIdList) const
{
    propIdList.clear();

    if (docId >= indexTable_.size())
        return;

    index_t index = indexTable_[docId];

    if (index & INDEX_MSB)
    {
        index &= INDEX_MASK;
        typename std::vector<valueid_t>::const_iterator it =
            multiValueTable_.begin() + index;

        propIdList.size_ = *it;
        propIdList.multiValueIter_ = ++it;
    }
    else if (index)
    {
        propIdList.size_ = 1;
        propIdList.singleValueId_ = index;
    }
}

template <typename valueid_t, typename index_t>
void PropIdTable<valueid_t, index_t>::appendIdList(const std::vector<valueid_t>& inputIdList)
{
    std::size_t count = inputIdList.size();

    switch (count)
    {
    case 0:
        indexTable_.push_back(0);
        break;

    case 1:
        indexTable_.push_back(inputIdList.front());
        break;

    default:
        index_t index = INDEX_MSB | multiValueTable_.size();
        indexTable_.push_back(index);

        multiValueTable_.push_back(count);
        multiValueTable_.insert(multiValueTable_.end(),
            inputIdList.begin(), inputIdList.end());
        break;
    }
}

NS_FACETED_END

#endif // SF1R_PROP_ID_TABLE_H

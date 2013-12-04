#ifndef SF1R_ZAMBEZI_FILTER_H
#define SF1R_ZAMBEZI_FILTER_H

#include <document-manager/DocumentManager.h>
#include <mining-manager/group-manager/GroupFilter.h>
#include <ir/index_manager/utility/Bitset.h>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

namespace sf1r
{

template <bool State>
class MonomorphicFilter
{
public:
    MonomorphicFilter() {}

    bool test(uint32_t docId) const
    {
        return State;
    }

    uint32_t skipTo(uint32_t docId) const
    {
        return docId;
    }
};

class ZambeziFilter
{
public:
    ZambeziFilter(
            const DocumentManager& documentManager,
            const boost::shared_ptr<faceted::GroupFilter>& groupFilter,
            const boost::shared_ptr<izenelib::ir::indexmanager::Bitset>& filterBitset)
        : current_(0)
        , documentManager_(documentManager)
        , groupFilter_(groupFilter)
        , filterBitset_(filterBitset)
    {
    }

    bool test(uint32_t docId) const
    {
        return !documentManager_.isDeleted(docId) &&
            (!filterBitset_ || filterBitset_->test(docId)) &&
            (!groupFilter_ || groupFilter_->test(docId));
    }

    uint32_t next() const
    {
        return current_;
    }

    uint32_t skipTo(uint32_t docId) const
    {
        return docId;
    }

private:
    bool reverse_;
    uint32_t current_;
    const DocumentManager& documentManager_;
    boost::shared_ptr<faceted::GroupFilter> groupFilter_;
    boost::shared_ptr<izenelib::ir::indexmanager::Bitset> filterBitset_;
};

} // namespace sf1r

#endif // SF1R_ZAMBEZI_FILTER_H

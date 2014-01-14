#ifndef SF1R_ZAMBEZI_FILTER_H
#define SF1R_ZAMBEZI_FILTER_H

#include <document-manager/DocumentManager.h>
#include <mining-manager/group-manager/GroupFilter.h>
#include <ir/index_manager/utility/Bitset.h>
#include <ir/Zambezi/FilterBase.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

namespace sf1r
{

class ZambeziFilter : public izenelib::ir::Zambezi::FilterBase
{
public:
    ZambeziFilter(
            const DocumentManager& documentManager,
            const boost::shared_ptr<faceted::GroupFilter>& groupFilter,
            const boost::shared_ptr<izenelib::ir::indexmanager::Bitset>& filterBitset)
        : documentManager_(documentManager)
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

    uint32_t find_first(bool reverse) const
    {
        if (filterBitset_)
        {
            return reverse ? (uint32_t)filterBitset_->find_last() : (uint32_t)filterBitset_->find_first();
        }
        else
        {
            return reverse ? documentManager_.getMaxDocId() : 1;
        }
    }

    uint32_t find_next(uint32_t id, bool reverse) const
    {
        if (filterBitset_)
        {
            return reverse ? (uint32_t)filterBitset_->find_prev(id) : (uint32_t)filterBitset_->find_next(id);
        }
        else
        {
            return reverse ? id - 1 : id + 1;
        }
    }

private:
    bool reverse_;
    const DocumentManager& documentManager_;
    boost::shared_ptr<faceted::GroupFilter> groupFilter_;
    boost::shared_ptr<izenelib::ir::indexmanager::Bitset> filterBitset_;
};

} // namespace sf1r

#endif // SF1R_ZAMBEZI_FILTER_H

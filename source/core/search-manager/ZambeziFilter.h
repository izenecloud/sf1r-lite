#ifndef SF1R_ZAMBEZI_FILTER_H
#define SF1R_ZAMBEZI_FILTER_H

#include <document-manager/DocumentManager.h>
#include <mining-manager/group-manager/GroupFilter.h>
#include <ir/index_manager/utility/BitVector.h>
#include <boost/shared_ptr.hpp>

namespace sf1r
{

class ZambeziFilter
{
public:
    ZambeziFilter(
            const DocumentManager& documentManager,
            const boost::shared_ptr<faceted::GroupFilter>& groupFilter,
            const boost::shared_ptr<izenelib::ir::indexmanager::BitVector>& filterBitVector)
        : documentManager_(documentManager)
        , groupFilter_(groupFilter)
        , filterBitVector_(filterBitVector)
    {
    }

    bool test(uint32_t docId) const
    {
        return !documentManager_.isDeleted(docId, false) &&
            (!groupFilter_ || groupFilter_->test(docId)) &&
            (!filterBitVector_ || filterBitVector_->test(docId));
    }

private:
    const DocumentManager& documentManager_;
    boost::shared_ptr<faceted::GroupFilter> groupFilter_;
    boost::shared_ptr<izenelib::ir::indexmanager::BitVector> filterBitVector_;
};

} // namespace sf1r

#endif // SF1R_ZAMBEZI_FILTER_H

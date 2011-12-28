///
/// @file StringGroupLabel.h
/// @brief filter docs with selected label on string property
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2011-07-29
///

#ifndef SF1R_STRING_GROUP_LABEL_H
#define SF1R_STRING_GROUP_LABEL_H

#include "GroupLabel.h"
#include "prop_value_table.h"
#include "GroupParam.h"

#include <vector>
#include <string>

NS_FACETED_BEGIN

class StringGroupLabel : public GroupLabel
{
public:
    StringGroupLabel(
        const GroupParam::GroupPathVec& labelPaths,
        const PropValueTable& pvTable
    );

    bool test(docid_t doc) const;

private:
    void getTargetValueIds_(const GroupParam::GroupPathVec& labelPaths);

private:
    const PropValueTable& propValueTable_;
    std::vector<PropValueTable::pvid_t> targetValueIds_;
    NS_BOOST_MEMORY::block_pool recycle_;
    boost::scoped_alloc alloc_;
    mutable PropValueTable::ParentSetType parentSet_;
};

NS_FACETED_END

#endif 

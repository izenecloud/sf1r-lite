///
/// @file StringGroupCounter.h
/// @brief count docs for string property values
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2011-07-29
///

#ifndef SF1R_STRING_GROUP_COUNTER_H
#define SF1R_STRING_GROUP_COUNTER_H

#include "GroupCounter.h"
#include "prop_value_table.h"

#include <vector>

NS_FACETED_BEGIN

class OntologyRepItem;

class StringGroupCounter : public GroupCounter
{
public:
    StringGroupCounter(const PropValueTable& pvTable);

    void addDoc(docid_t doc);

    void getGroupRep(GroupRep& groupRep);

private:
    /**
     * append all descendent nodes of @p pvId to @p itemList.
     * @param itemList the nodes are appended to this list
     * @param pvId the value id of the root node to append
     * @param level the level of the root node to append
     * @param valueStr the string value of the root node
     */
    void appendGroupRep(
        std::list<OntologyRepItem>& itemList,
        PropValueTable::pvid_t pvId,
        int level,
        const izenelib::util::UString& valueStr
    ) const;

private:
    const PropValueTable& propValueTable_;

    const std::vector<PropValueTable::PropStrMap>& childMapTable_;

    /** map from value id to doc count */
    std::vector<int> countTable_;
    //NS_BOOST_MEMORY::block_pool recycle_;
    //boost::scoped_alloc alloc_;
    mutable PropValueTable::ParentSetType parentSet_;
};

NS_FACETED_END

#endif 

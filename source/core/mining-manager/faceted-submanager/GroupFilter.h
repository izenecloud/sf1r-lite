///
/// @file GroupFilter.h
/// @brief filter docs which do not belong to selected labels
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2011-07-28
///

#ifndef SF1R_GROUP_FILTER_H
#define SF1R_GROUP_FILTER_H

#include "faceted_types.h"

NS_FACETED_BEGIN

class GroupManager;
class AttrTable;
class GroupParam;
class OntologyRep;
class GroupRep;
class GroupLabel;
class AttrLabel;
class GroupCounter;
class AttrCounter;
class GroupCounterLabelBuilder;

class GroupFilter
{
public:
    GroupFilter(const GroupParam& groupParam);

    ~GroupFilter();

    /**
     * @return true for success, false for failure
     */
    bool initGroup(GroupCounterLabelBuilder& builder);

    /**
     * @return true for success, false for failure
     */
    bool initAttr(const AttrTable* attrTable);

    /**
     * Check whether doc belongs to the labels in @c groupParam_.
     * @param doc doc id to check
     * @return true for @p doc belongs to the labels, false for not belongs to the labels.
     *         if the labels in @c groupParam_ are empty, true is returned.
     */
    bool test(docid_t doc);

    /**
     * Get doc counts for property values and attribute values,
     * it counts the param @p doc in previous calls of @c test().
     * @param groupRep doc counts for property values
     * @param attrRep doc counts for attribute values
     */
    void getGroupRep(
        GroupRep& groupRep,
        OntologyRep& attrRep
    );

private:
    const GroupParam& groupParam_;

    /** group label instances */
    std::vector<GroupLabel*> groupLabels_;

    /** group counter instances */
    std::vector<GroupCounter*> groupCounters_;

    /** attr label instances */
    std::vector<AttrLabel*> attrLabels_;

    /** attr counter instance */
    AttrCounter* attrCounter_;
};

NS_FACETED_END

#endif

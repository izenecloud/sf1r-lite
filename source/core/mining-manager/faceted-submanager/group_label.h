///
/// @file group_label.h
/// @brief check whether a doc belongs to the labels selected.
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2011-06-24
///

#ifndef SF1R_GROUP_LABEL_H_
#define SF1R_GROUP_LABEL_H_

#include "prop_value_table.h"

#include <vector>
#include <string>

NS_FACETED_BEGIN

class GroupLabel {
public:
    /** a condition pair of value table and target value id */
    typedef std::pair<const PropValueTable*, PropValueTable::pvid_t> CondPair;
    typedef std::vector<CondPair> CondList;

    GroupLabel(const CondList& condList)
        : condList_(condList)
    {}

    /**
     * check whether @p docId meets the conditions in @c condList_.
     * @param docId the doc id to check
     * @return true for meet the conditions, false for not meet the conditions.
     */
    bool checkDoc(docid_t docId) const;

    /**
     * check whether @p docId meets the conditions in @c condList_,
     * while not to check on property @p propName,
     * as in @c GroupManager::getGroupRep(), for each label, we need to keep the docs belongs to other labels.
     * @param docId the doc id to check
     * @param propName the property name not to check
     * @return true for meet the conditions, false for not meet the conditions.
     */
    bool checkDocExceptProp(
        docid_t docId,
        const std::string& propName
    ) const;

private:
    CondList condList_;
};

NS_FACETED_END

#endif //SF1R_GROUP_LABEL_H_

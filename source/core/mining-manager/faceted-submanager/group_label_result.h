///
/// @file group_label_result.h
/// @brief from group results, get doc id list from one label
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2011-06-24
///

#ifndef SF1R_GROUP_LABEL_RESULT_H_
#define SF1R_GROUP_LABEL_RESULT_H_

#include "ontology_rep.h"
#include <util/ustring/UString.h>

#include <boost/bind.hpp>

#include <vector>
#include <string>
#include <algorithm> // find_if
#include <cassert>

NS_FACETED_BEGIN

class GroupLabelResult {
public:
    GroupLabelResult(const OntologyRep& groupRep)
        : groupRep_(groupRep)
    {}

    /**
     * get doc id list on the label.
     * @param labelName the property name or attribute name
     * @param labelValue the property value or attribute value
     * @return the doc id list, if no label is found, @c emptyDocList is returned.
     */
    const std::vector<docid_t>& selectLabel(
        const std::string& labelName,
        const std::string& labelValue
    ) const
    {
        assert(!labelName.empty() && !labelValue.empty());

        izenelib::util::UString propName(labelName, izenelib::util::UString::UTF_8);
        izenelib::util::UString propValue(labelValue, izenelib::util::UString::UTF_8);

        typedef std::list<sf1r::faceted::OntologyRepItem> GroupRepList;
        const GroupRepList& groupList = groupRep_.item_list;
        GroupRepList::const_iterator groupIt =
            std::find_if(groupList.begin(), groupList.end(),
                boost::bind(&sf1r::faceted::OntologyRepItem::level, _1) == 0 &&
                boost::bind(&sf1r::faceted::OntologyRepItem::text, _1) == propName);

        if (groupIt != groupList.end())
        {
            ++groupIt;
            groupIt = std::find_if(groupIt, groupList.end(),
                boost::bind(&sf1r::faceted::OntologyRepItem::level, _1) == 0 ||
                boost::bind(&sf1r::faceted::OntologyRepItem::text, _1) == propValue);

            if (groupIt != groupList.end() && groupIt->level != 0)
            {
                return groupIt->doc_id_list;
            }
        }

        return emptyDocList;
    }

private:
    const OntologyRep& groupRep_;
    const std::vector<docid_t> emptyDocList;
};

NS_FACETED_END

#endif //SF1R_GROUP_LABEL_RESULT_H_

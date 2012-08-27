///
/// @file AttrLabel.h
/// @brief filter docs on selected attribute label
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2011-07-29
///

#ifndef SF1R_ATTR_LABEL_H
#define SF1R_ATTR_LABEL_H

#include "AttrTable.h"
#include "../faceted-submanager/faceted_types.h"
#include <set>

NS_FACETED_BEGIN

class AttrLabel
{
public:
    AttrLabel(
        const AttrTable& attrTable,
        const std::string& attrName,
        const std::vector<std::string>& attrValues
    );

    bool test(docid_t doc) const;

    AttrTable::nid_t attrNameId() const;

private:
    void getTargetValueIds_(const std::vector<std::string>& attrValues);

private:
    const AttrTable& attrTable_;

    const AttrTable::nid_t attrNameId_;
    std::set<AttrTable::vid_t> targetValueIds_;
    std::set<AttrTable::vid_t>::const_iterator targetIterEnd_;
};

NS_FACETED_END

#endif 

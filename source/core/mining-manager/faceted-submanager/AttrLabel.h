///
/// @file AttrLabel.h
/// @brief filter docs on selected attribute label
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2011-07-29
///

#ifndef SF1R_ATTR_LABEL_H
#define SF1R_ATTR_LABEL_H

#include "faceted_types.h"
#include "attr_table.h"

NS_FACETED_BEGIN

class AttrLabel
{
public:
    AttrLabel(
        const std::pair<std::string, std::string>& label,
        const AttrTable* attrTable
    );

    bool test(docid_t doc) const;

    AttrTable::nid_t attrNameId() const;

private:
    const AttrTable::ValueIdTable& valueIdTable_;
    const AttrTable::nid_t attrNameId_;
    const AttrTable::vid_t targetValueId_;
};

NS_FACETED_END

#endif 

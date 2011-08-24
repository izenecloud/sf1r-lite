///
/// @file GroupParam.h
/// @brief parameters for group filter
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2011-07-29
///

#ifndef SF1R_GROUP_PARAM_H
#define SF1R_GROUP_PARAM_H

#include "faceted_types.h"

#include <util/izene_serialization.h>
#include <3rdparty/msgpack/msgpack.hpp>
#include <boost/serialization/access.hpp>

#include <vector>
#include <string>
#include <iostream>
#include <utility> // pair

NS_FACETED_BEGIN

struct GroupParam
{
    GroupParam();

    /** the property names which need doc counts for each property value */
    std::vector<std::string> groupProps_;

    /** the group path contains values from root to leaf node */
    typedef std::vector<std::string> GroupPath;
    /** the group label contains property name and group path */
    typedef std::pair<std::string, GroupPath> GroupLabel;
    /** a list of group labels */
    typedef std::vector<GroupLabel> GroupLabelVec;
    /** selected group labels */
    GroupLabelVec groupLabels_;

    /** true for need doc counts for each attribute value */
    bool isAttrGroup_;

    /** the number of attributes to return */
    int attrGroupNum_;

    /** the attribute label contains a pair of attribute name and value */
    typedef std::pair<std::string, std::string> AttrLabel;
    /** a list of attribute labels */
    typedef std::vector<AttrLabel> AttrLabelVec;
    /** selected attribute labels */
    AttrLabelVec attrLabels_;

    /**
     * Whether need to filter docs by group filter.
     * @return true for empty group params, not need group filter,
     *         false for group params not empty, need group filter.
     */
    bool empty() const;

    DATA_IO_LOAD_SAVE(GroupParam, &groupProps_&groupLabels_
            &isAttrGroup_&attrGroupNum_&attrLabels_);

    MSGPACK_DEFINE(groupProps_, groupLabels_,
            isAttrGroup_, attrGroupNum_, attrLabels_);

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & groupProps_;
        ar & groupLabels_;
        ar & isAttrGroup_;
        ar & attrGroupNum_;
        ar & attrLabels_;
    }
};

bool operator==(const GroupParam& a, const GroupParam& b);
std::ostream& operator<<(std::ostream& out, const GroupParam& groupParam);

NS_FACETED_END

#endif 

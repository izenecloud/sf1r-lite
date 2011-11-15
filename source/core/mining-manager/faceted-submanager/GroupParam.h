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
#include <map>
#include <string>
#include <iostream>
#include <utility> // pair

namespace sf1r
{
class MiningSchema;
class GroupConfig;
}

NS_FACETED_BEGIN

struct GroupPropParam
{
    std::string property_;
    bool isRange_;

    GroupPropParam();

    DATA_IO_LOAD_SAVE(GroupPropParam, &property_&isRange_);

    MSGPACK_DEFINE(property_, isRange_);

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & property_;
        ar & isRange_;
    }
};
bool operator==(const GroupPropParam& a, const GroupPropParam& b);
std::ostream& operator<<(std::ostream& out, const GroupPropParam& groupPropParam);

struct GroupParam
{
    GroupParam();

    /** the properties which need doc counts for each property value */
    std::vector<GroupPropParam> groupProps_;

    /** the group path contains values from root to leaf node */
    typedef std::vector<std::string> GroupPath;
    /** a list of group paths for one property */
    typedef std::vector<GroupPath> GroupPathVec;
    /** map from property name to group paths */
    typedef std::map<std::string, GroupPathVec> GroupLabelMap;
    /** a pair of property name and group path */
    typedef GroupLabelMap::value_type GroupLabelParam;
    /** selected group labels */
    GroupLabelMap groupLabels_;

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

    bool isEmpty() const;
    bool isGroupEmpty() const;
    bool isAttrEmpty() const;
    bool checkParam(const MiningSchema& miningSchema, std::string& message) const;

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

private:
    bool checkGroupParam_(const MiningSchema& miningSchema, std::string& message) const;
    bool checkGroupProps_(const std::vector<GroupConfig>& groupProps, std::string& message) const;
    bool checkGroupLabels_(const std::vector<GroupConfig>& groupProps, std::string& message) const;
    bool checkAttrParam_(const MiningSchema& miningSchema, std::string& message) const;
    bool isRangeLabel_(const std::string& propName) const;
};

bool operator==(const GroupParam& a, const GroupParam& b);
std::ostream& operator<<(std::ostream& out, const GroupParam& groupParam);

NS_FACETED_END

#endif

///
/// @file GroupParam.h
/// @brief parameters for group filter
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2011-07-29
///

#ifndef SF1R_GROUP_PARAM_H
#define SF1R_GROUP_PARAM_H

#include "../faceted-submanager/faceted_types.h"
#include <configuration-manager/GroupConfig.h>

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
}

NS_FACETED_BEGIN

struct GroupPropParam
{
    std::string property_;
    std::string subProperty_;
    bool isRange_;
    std::string unit_;
    int group_top_;

    GroupPropParam();

    DATA_IO_LOAD_SAVE(GroupPropParam, &property_&subProperty_&isRange_&unit_&group_top_);

    MSGPACK_DEFINE(property_, subProperty_, isRange_, unit_, group_top_);

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & property_;
        ar & subProperty_;
        ar & isRange_;
        ar & unit_;
        ar & group_top_;
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

    /** property name => at most how many group labels are auto selected */
    typedef std::map<std::string, int> AutoSelectLimitMap;
    /** group labels to auto select */
    AutoSelectLimitMap autoSelectLimits_;

    /** true for need doc counts for each attribute value */
    bool isAttrGroup_;

    /** the number of attributes to return */
    int attrGroupNum_;

    /** a list of attribute values for one attribute name */
    typedef std::vector<std::string> AttrValueVec;
    /** map from attribute name to attribute values */
    typedef std::map<std::string, AttrValueVec> AttrLabelMap;
    /** selected attribute labels */
    AttrLabelMap attrLabels_;

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
    bool checkGroupProps_(const GroupConfigMap& groupConfigMap, std::string& message) const;
    bool checkGroupLabels_(const GroupConfigMap& groupConfigMap, std::string& message) const;
    bool checkAutoSelectLimits_(const GroupConfigMap& groupConfigMap, std::string& message) const;
    bool checkAttrParam_(const MiningSchema& miningSchema, std::string& message) const;
    bool isRangeLabel_(const std::string& propName) const;
};

bool operator==(const GroupParam& a, const GroupParam& b);
std::ostream& operator<<(std::ostream& out, const GroupParam& groupParam);
std::ostream& operator<<(std::ostream& out, const GroupParam::GroupLabelMap& groupLabelMap);

NS_FACETED_END

MAKE_FEBIRD_SERIALIZATION(sf1r::faceted::GroupParam)
#endif

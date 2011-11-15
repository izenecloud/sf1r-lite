#include "GroupParam.h"

namespace
{
const char* NUMERIC_RANGE_DELIMITER = "-";
}

NS_FACETED_BEGIN

GroupPropParam::GroupPropParam()
    : isRange_(false)
{
}

bool operator==(const GroupPropParam& a, const GroupPropParam& b)
{
    return a.property_ == b.property_
        && a.isRange_ == b.isRange_;
}

std::ostream& operator<<(std::ostream& out, const GroupPropParam& groupPropParam)
{
    out << "property: " << groupPropParam.property_
        << ", is range: " << groupPropParam.isRange_
        << std::endl;

    return out;
}

GroupParam::GroupParam()
    : isAttrGroup_(false)
    , attrGroupNum_(0)
{
}

bool GroupParam::isEmpty() const
{
    return isGroupEmpty() && isAttrEmpty();
}

bool GroupParam::isGroupEmpty() const
{
    return groupProps_.empty() && groupLabels_.empty();
}

bool GroupParam::isAttrEmpty() const
{
    return isAttrGroup_ == false && attrLabels_.empty();
}

bool GroupParam::checkParam(std::string& message) const
{
    for (std::vector<GroupPropParam>::const_iterator propIt = groupProps_.begin();
        propIt != groupProps_.end(); ++propIt)
    {
        const std::string& propName = propIt->property_;
        if (propIt->isRange_ && isRangeLabel_(propName))
        {
            message = "the property " + propName + " in request[search][group_label] could not be specified in request[group] at the same time";
            return false;
        }
    }

    return true;
}

bool GroupParam::isRangeLabel_(const std::string& propName) const
{
    GroupLabelMap::const_iterator findIt = groupLabels_.find(propName);
    if (findIt == groupLabels_.end())
        return false;

    const GroupPathVec& paths = findIt->second;
    for (GroupPathVec::const_iterator pathIt = paths.begin();
        pathIt != paths.end(); ++pathIt)
    {
        if (!pathIt->empty()
            && pathIt->front().find(NUMERIC_RANGE_DELIMITER) != std::string::npos)
        {
            return true;
        }
    }

    return false;
}

bool operator==(const GroupParam& a, const GroupParam& b)
{
    return a.groupProps_ == b.groupProps_
        && a.groupLabels_ == b.groupLabels_
        && a.isAttrGroup_ == b.isAttrGroup_
        && a.attrGroupNum_ == b.attrGroupNum_
        && a.attrLabels_ == b.attrLabels_;
}

std::ostream& operator<<(std::ostream& out, const GroupParam& groupParam)
{
    out << "groupProps_: ";
    for (std::size_t i = 0; i < groupParam.groupProps_.size(); ++i)
    {
        out << groupParam.groupProps_[i];
    }

    out << "groupLabels_: ";
    for (GroupParam::GroupLabelMap::const_iterator labelIt = groupParam.groupLabels_.begin();
        labelIt != groupParam.groupLabels_.end(); ++labelIt)
    {
        const std::string& propName = labelIt->first;
        const GroupParam::GroupPathVec& pathVec = labelIt->second;

        for (GroupParam::GroupPathVec::const_iterator pathIt = pathVec.begin();
            pathIt != pathVec.end(); ++pathIt)
        {
            out << "\t" << propName << ": ";
            for (GroupParam::GroupPath::const_iterator nodeIt = pathIt->begin();
                nodeIt != pathIt->end(); ++nodeIt)
            {
                out << *nodeIt << ", ";
            }
            out << std::endl;
        }
    }

    out << "isAttrGroup_: " << groupParam.isAttrGroup_ << std::endl;
    out << "attrGroupNum_: " << groupParam.attrGroupNum_ << std::endl;

    out << "attrLabels_: ";
    for (std::size_t i = 0; i < groupParam.attrLabels_.size(); ++i)
    {
        const GroupParam::AttrLabel& attrLabel = groupParam.attrLabels_[i];
        out << "(" << attrLabel.first << ", " << attrLabel.second << "), ";
    }
    out << std::endl;

    return out;
}

NS_FACETED_END

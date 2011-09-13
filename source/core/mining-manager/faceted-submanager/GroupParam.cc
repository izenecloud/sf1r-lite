#include "GroupParam.h"

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
    for (GroupLabelVec::const_iterator labelIt = groupLabels_.begin();
        labelIt != groupLabels_.end(); ++labelIt)
    {
        const std::string& propName = labelIt->first;
        const GroupPath& groupPath = labelIt->second;
        if (groupPath.empty())
        {
            message = "request[search][group_label][value] is empty for property " + propName;
            return false;
        }

        // range label
        if (groupPath[0].find('-') != std::string::npos)
        {
            for (std::vector<GroupPropParam>::const_iterator propIt = groupProps_.begin();
                propIt != groupProps_.end(); ++propIt)
            {
                if (propIt->property_ == propName && propIt->isRange_)
                {
                    message = "the property " + propName + " in request[search][group_label] could not be specified in request[group] at the same time";
                    return false;
                }
            }
        }
    }

    return true;
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
    for (std::size_t i = 0; i < groupParam.groupLabels_.size(); ++i)
    {
        const GroupParam::GroupLabel& groupLabel = groupParam.groupLabels_[i];
        out << "\t" << groupLabel.first << ": ";
        const GroupParam::GroupPath& groupPath = groupLabel.second;
        for (std::size_t j = 0; j < groupPath.size(); ++j)
        {
            out << groupPath[j] << ", ";
        }
        out << std::endl;
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

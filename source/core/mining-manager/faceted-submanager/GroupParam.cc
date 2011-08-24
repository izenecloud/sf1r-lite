#include "GroupParam.h"

NS_FACETED_BEGIN

GroupParam::GroupParam()
    : isAttrGroup_(false)
    , attrGroupNum_(0)
{
}

bool GroupParam::empty() const
{
    if (groupProps_.empty() && isAttrGroup_ == false)
        return true;

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
        out << groupParam.groupProps_[i] << ", ";
    }
    out << std::endl;

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

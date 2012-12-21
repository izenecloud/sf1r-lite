#include "GroupParam.h"
#include "DateStrParser.h"
#include <configuration-manager/MiningSchema.h>

namespace
{
const char* NUMERIC_RANGE_DELIMITER = "-";

using namespace sf1r;
using namespace sf1r::faceted;

bool checkGroupPropParam(
    const GroupPropParam& param,
    const GroupConfigMap& configMap,
    std::string& message
)
{
    const std::string& propName = param.property_;
    const std::string& subPropName = param.subProperty_;
    const std::string& unit = param.unit_;

    if (propName.empty())
    {
        message = "request[group][property] should not be empty";
        return false;
    }

    GroupConfigMap::const_iterator configIt = configMap.find(propName);
    if (configIt == configMap.end())
    {
        message = "property " + propName + " should be configured in <MiningBundle>::<Schema>::<Group>.";
        return false;
    }
    const GroupConfig& groupConfig = configIt->second;

    if (! subPropName.empty())
    {
        GroupConfigMap::const_iterator subConfigIt = configMap.find(subPropName);
        if (subConfigIt == configMap.end())
        {
            message = "property " + subPropName + " should be configured in <MiningBundle>::<Schema>::<Group>.";
            return false;
        }
        const GroupConfig& subConfig = subConfigIt->second;

        if (subPropName == propName)
        {
            message = "property " + subPropName + " in request[group][sub_property] should not be duplicated with request[group][property].";
            return false;
        }

        if (subConfig.isDateTimeType())
        {
            message = "request[group][sub_property] does not support datetime property " + subPropName;
            return false;
        }
    }

    if (param.isRange_)
    {
        if (! subPropName.empty())
        {
            message = "request[group][range] must be false when request[group][sub_property] is set.";
            return false;
        }

        if (! groupConfig.isNumericType())
        {
            message = "property type of " + propName + " should be int or float when request[group][range] is true.";
            return false;
        }
    }

    if (!groupConfig.isDateTimeType() &&
        !unit.empty())
    {
        message = "as property type of " + propName + " is not datetime, request[group][unit] should be empty.";
        return false;
    }

    if (groupConfig.isDateTimeType())
    {
        if (unit.empty())
        {
            message = "as property type of " + propName + " is datetime, request[group][unit] should not be empty.";
            return false;
        }

        DATE_MASK_TYPE mask;
        if (! DateStrParser::get()->unitStrToMask(unit, mask, message))
            return false;
    }

    return true;
}

bool checkDateLabel(
    const GroupParam::GroupPathVec& groupPathVec,
    std::string& message
)
{
    DateStrParser* dateStrParser = DateStrParser::get();

    for (GroupParam::GroupPathVec::const_iterator pathIt = groupPathVec.begin();
            pathIt != groupPathVec.end(); ++pathIt)
    {
        const GroupParam::GroupPath& path = *pathIt;
        if (path.empty())
        {
            message = "Must specify non-empty [search][group_label][value]";
            return false;
        }

        DateStrParser::DateMask dateMask;
        if (! dateStrParser->apiStrToDateMask(path.front(), dateMask, message))
            return false;
    }

    return true;
}

} // namespace

NS_FACETED_BEGIN

GroupPropParam::GroupPropParam()
    : isRange_(false),
    group_top_(0)
{
}

bool operator==(const GroupPropParam& a, const GroupPropParam& b)
{
    return a.property_ == b.property_ &&
           a.subProperty_ == b.subProperty_ &&
           a.isRange_ == b.isRange_ &&
           a.unit_ == b.unit_ &&
           a.group_top_ == b.group_top_;
}

std::ostream& operator<<(std::ostream& out, const GroupPropParam& groupPropParam)
{
    out << "property: " << groupPropParam.property_
        << ", sub property: " << groupPropParam.subProperty_
        << ", is range: " << groupPropParam.isRange_
        << ", unit: " << groupPropParam.unit_
        << ", group top: " << groupPropParam.group_top_
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
    return groupProps_.empty() && groupLabels_.empty() &&
        autoSelectLimits_.empty();
}

bool GroupParam::isAttrEmpty() const
{
    return isAttrGroup_ == false && attrLabels_.empty();
}

bool GroupParam::checkParam(const MiningSchema& miningSchema, std::string& message) const
{
    return checkGroupParam_(miningSchema, message) &&
           checkAttrParam_(miningSchema, message);
}

bool GroupParam::checkGroupParam_(const MiningSchema& miningSchema, std::string& message) const
{
    if (isGroupEmpty())
        return true;

    if (! miningSchema.group_enable)
    {
        message = "The GroupBy properties have not been configured in <MiningBundle>::<Schema>::<Group> yet.";
        return false;
    }

    return checkGroupProps_(miningSchema.group_config_map, message) &&
           checkGroupLabels_(miningSchema.group_config_map, message) &&
           checkAutoSelectLimits_(miningSchema.group_config_map, message);
}

bool GroupParam::checkGroupProps_(const GroupConfigMap& groupConfigMap, std::string& message) const
{
    for (std::vector<GroupPropParam>::const_iterator paramIt = groupProps_.begin();
        paramIt != groupProps_.end(); ++paramIt)
    {
        const std::string& propName = paramIt->property_;

        if (! checkGroupPropParam(*paramIt, groupConfigMap, message))
            return false;

        if (paramIt->isRange_ && isRangeLabel_(propName))
        {
            message = "property " + propName + " in request[search][group_label] could not be specified in request[group] at the same time";
            return false;
        }
    }

    return true;
}

bool GroupParam::checkGroupLabels_(const GroupConfigMap& groupConfigMap, std::string& message) const
{
    for (GroupLabelMap::const_iterator labelIt = groupLabels_.begin();
        labelIt != groupLabels_.end(); ++labelIt)
    {
        const std::string& propName = labelIt->first;

        GroupConfigMap::const_iterator configIt = groupConfigMap.find(propName);
        if (configIt == groupConfigMap.end())
        {
            message = "property " + propName + " should be configured in <MiningBundle>::<Schema>::<Group>.";
            return false;
        }

        const GroupConfig& groupConfig = configIt->second;
        if (groupConfig.isDateTimeType() &&
            !checkDateLabel(labelIt->second, message))
        {
            return false;
        }
    }

    return true;
}

bool GroupParam::checkAutoSelectLimits_(const GroupConfigMap& groupConfigMap, std::string& message) const
{
    for (AutoSelectLimitMap::const_iterator limitIt = autoSelectLimits_.begin();
        limitIt != autoSelectLimits_.end(); ++limitIt)
    {
        const std::string& propName = limitIt->first;

        GroupConfigMap::const_iterator configIt = groupConfigMap.find(propName);
        if (configIt == groupConfigMap.end())
        {
            message = "property " + propName + " should be configured in <MiningBundle>::<Schema>::<Group>.";
            return false;
        }

        if (! configIt->second.isStringType())
        {
            message = "the feature of auto selected label does not support the property '" + propName + "' other than string type";
            return false;
        }
    }

    return true;
}

bool GroupParam::checkAttrParam_(const MiningSchema& miningSchema, std::string& message) const
{
    if (isAttrEmpty())
        return true;

    if (! miningSchema.attr_enable)
    {
        message = "To get group results by attribute value, the attribute property should be configured in <MiningBundle>::<Schema>::<Attr>.";
        return false;
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
        if (!pathIt->empty() &&
            pathIt->front().find(NUMERIC_RANGE_DELIMITER) != std::string::npos)
        {
            return true;
        }
    }

    return false;
}

bool operator==(const GroupParam& a, const GroupParam& b)
{
    return a.groupProps_ == b.groupProps_ &&
           a.groupLabels_ == b.groupLabels_ &&
           a.autoSelectLimits_ == b.autoSelectLimits_ &&
           a.isAttrGroup_ == b.isAttrGroup_ &&
           a.attrGroupNum_ == b.attrGroupNum_ &&
           a.attrLabels_ == b.attrLabels_;
}

std::ostream& operator<<(std::ostream& out, const GroupParam& groupParam)
{
    out << "groupProps_: ";
    for (std::size_t i = 0; i < groupParam.groupProps_.size(); ++i)
    {
        out << groupParam.groupProps_[i];
    }

    out << "groupLabels_:" << std::endl;
    out << groupParam.groupLabels_ << std::endl;

    out << "autoSelectLimits_:" << std::endl;
    for (GroupParam::AutoSelectLimitMap::const_iterator limitIt = groupParam.autoSelectLimits_.begin();
         limitIt != groupParam.autoSelectLimits_.end(); ++limitIt)
    {
        const std::string& propName = limitIt->first;
        int limit = limitIt->second;
        out << "property " << propName
            << ", limit " << limit << " auto selected labels" << std::endl;
    }

    out << "isAttrGroup_: " << groupParam.isAttrGroup_ << std::endl;
    out << "attrGroupNum_: " << groupParam.attrGroupNum_ << std::endl;

    out << "attrLabels_: ";
    for (GroupParam::AttrLabelMap::const_iterator labelIt = groupParam.attrLabels_.begin();
         labelIt != groupParam.attrLabels_.end(); ++labelIt)
    {
        const std::string& attrName = labelIt->first;
        const GroupParam::AttrValueVec& valueVec = labelIt->second;

        out << "\t" << attrName << ": ";
        for (GroupParam::AttrValueVec::const_iterator valueIt = valueVec.begin();
             valueIt != valueVec.end(); ++valueIt)
        {
            out << *valueIt << ", ";
        }
        out << std::endl;
    }

    return out;
}

std::ostream& operator<<(std::ostream& out, const GroupParam::GroupLabelMap& groupLabelMap)
{
    for (GroupParam::GroupLabelMap::const_iterator labelIt = groupLabelMap.begin();
         labelIt != groupLabelMap.end(); ++labelIt)
    {
        const std::string& propName = labelIt->first;
        const GroupParam::GroupPathVec& pathVec = labelIt->second;

        for (GroupParam::GroupPathVec::const_iterator pathIt = pathVec.begin();
             pathIt != pathVec.end(); ++pathIt)
        {
            out << "property " << propName << ", "
                << pathIt->size() << " labels" << std::endl;

            for (GroupParam::GroupPath::const_iterator nodeIt = pathIt->begin();
                 nodeIt != pathIt->end(); ++nodeIt)
            {
                out << *nodeIt << ", ";
            }
            out << std::endl;
        }
    }

    return out;
}

NS_FACETED_END

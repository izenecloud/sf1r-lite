#include "GroupCounterLabelBuilder.h"
#include "GroupManager.h"
#include "SubGroupCounter.h"
#include "StringGroupCounter.h"
#include "StringGroupLabel.h"
#include "NumericGroupCounter.h"
#include "NumericRangeGroupCounter.h"
#include "NumericRangeGroupLabel.h"
#include "DateGroupCounter.h"
#include "DateGroupLabel.h"
#include "DateStrParser.h"
#include <configuration-manager/GroupConfig.h>
#include <search-manager/NumericPropertyTableBuilder.h>

#include <limits>
#include <boost/lexical_cast.hpp>
#include <boost/scoped_ptr.hpp>
#include <glog/logging.h>

namespace
{
const char* NUMERIC_RANGE_DELIMITER = "-";

using namespace sf1r::faceted;

/**
 * check parameter of group label
 * @param[in] labelParam parameter to check
 * @param[out] isRange true for group on range, false for group on single numeric value
 * @return true for success, false for failure
*/
bool checkLabelParam(const GroupParam::GroupLabelParam& labelParam, bool& isRange)
{
    const GroupParam::GroupPathVec& labelPaths = labelParam.second;

    if (labelPaths.empty())
        return false;

    const GroupParam::GroupPath& path = labelPaths.front();
    if (path.empty())
        return false;

    const std::string& propValue = path.front();
    std::size_t delimitPos = propValue.find(NUMERIC_RANGE_DELIMITER);
    isRange = (delimitPos != std::string::npos);

    return true;
}

bool convertNumericLabel(const std::string& src, float& target)
{
    std::size_t delimitPos = src.find(NUMERIC_RANGE_DELIMITER);
    if (delimitPos != std::string::npos)
    {
        LOG(ERROR) << "group label parameter: " << src
                   << ", it should be specified as single numeric value";
        return false;
    }

    try
    {
        target = boost::lexical_cast<float>(src);
    }
    catch(const boost::bad_lexical_cast& e)
    {
        LOG(ERROR) << "failed in casting label from " << src
                   << " to numeric value, exception: " << e.what();
        return false;
    }

    return true;
}

bool convertRangeLabel(const std::string& src, NumericRangeGroupLabel::NumericRange& target)
{
    std::size_t delimitPos = src.find(NUMERIC_RANGE_DELIMITER);
    if (delimitPos == std::string::npos)
    {
        LOG(ERROR) << "group label parameter: " << src
                   << ", it should be specified as numeric range value";
        return false;
    }

    int64_t lowerBound = std::numeric_limits<int64_t>::min();
    int64_t upperBound = std::numeric_limits<int64_t>::max();

    try
    {
        if (delimitPos)
        {
            std::string sub = src.substr(0, delimitPos);
            lowerBound = boost::lexical_cast<int64_t>(sub);
        }

        if (delimitPos+1 != src.size())
        {
            std::string sub = src.substr(delimitPos+1);
            upperBound = boost::lexical_cast<int64_t>(sub);
        }
    }
    catch(const boost::bad_lexical_cast& e)
    {
        LOG(ERROR) << "failed in casting label from " << src
                    << " to numeric value, exception: " << e.what();
        return false;
    }

    target.first = lowerBound;
    target.second = upperBound;

    return true;
}

}

NS_FACETED_BEGIN

GroupCounterLabelBuilder::GroupCounterLabelBuilder(
        const std::vector<GroupConfig>& groupConfigs,
        const GroupManager* groupManager,
        NumericPropertyTableBuilder* numericTableBuilder)
    : groupConfigs_(groupConfigs)
    , groupManager_(groupManager)
    , numericTableBuilder_(numericTableBuilder)
{
}

PropertyDataType GroupCounterLabelBuilder::getPropertyType_(const std::string& prop) const
{
    for (std::vector<GroupConfig>::const_iterator it = groupConfigs_.begin();
        it != groupConfigs_.end(); ++it)
    {
        if (it->propName == prop)
            return it->propType;
    }

    return UNKNOWN_DATA_PROPERTY_TYPE;
}

const GroupConfig* GroupCounterLabelBuilder::getGroupConfig_(const std::string& prop) const
{
    for (std::vector<GroupConfig>::const_iterator it = groupConfigs_.begin();
        it != groupConfigs_.end(); ++it)
    {
        if (it->propName == prop)
            return &(*it);
    }

    return NULL;
}

GroupCounter* GroupCounterLabelBuilder::createGroupCounter(const GroupPropParam& groupPropParam)
{
    GroupCounter* counter = NULL;

    if (groupPropParam.isRange_)
    {
        counter = createNumericRangeCounter_(groupPropParam.property_);
    }
    else
    {
        const std::string& subPropName = groupPropParam.subProperty_;
        if (subPropName.empty())
        {
            counter = createValueCounter_(groupPropParam);
        }
        else
        {
            GroupPropParam subPropParam;
            subPropParam.property_ = subPropName;

            boost::scoped_ptr<GroupCounter> subCounter(createValueCounter_(subPropParam));
            counter = createValueCounter_(groupPropParam, subCounter.get());
        }
    }

    return counter;
}

GroupCounter* GroupCounterLabelBuilder::createValueCounter_(const GroupPropParam& groupPropParam, GroupCounter* subCounter) const
{
    GroupCounter* counter = NULL;
    const std::string& prop = groupPropParam.property_;

    PropertyDataType type = getPropertyType_(prop);
    switch(type)
    {
    case STRING_PROPERTY_TYPE:
        counter = createStringCounter_(prop, subCounter);
        break;

    case INT32_PROPERTY_TYPE:
    case FLOAT_PROPERTY_TYPE:
    case INT64_PROPERTY_TYPE:
    case DOUBLE_PROPERTY_TYPE:
         counter = createNumericCounter_(prop, subCounter);
         break;

    case DATETIME_PROPERTY_TYPE:
         counter = createDateCounter_(prop, groupPropParam.unit_, subCounter);
         break;

    default:
        LOG(ERROR) << "unsupported type " << type
                   << " for group property " << prop;
        break;
    }

    return counter;
}

GroupCounter* GroupCounterLabelBuilder::createStringCounter_(const std::string& prop, GroupCounter* subCounter) const
{
    GroupCounter* counter = NULL;
    const PropValueTable* pvTable = groupManager_->getPropValueTable(prop);

    if (pvTable)
    {
        if (subCounter)
        {
            SubGroupCounter subGroupCounter(subCounter);
            counter = new StringGroupCounter<SubGroupCounter>(*pvTable, subGroupCounter);
        }
        else
        {
            counter = new StringGroupCounter<>(*pvTable);
        }
    }
    else
    {
        LOG(ERROR) << "group index file is not loaded for group property " << prop;
    }

    return counter;
}

GroupCounter* GroupCounterLabelBuilder::createNumericCounter_(const std::string& prop, GroupCounter* subCounter) const
{
    GroupCounter* counter = NULL;
    boost::shared_ptr<NumericPropertyTableBase>& numericPropertyTable = numericTableBuilder_->createPropertyTable(prop);

    if (numericPropertyTable)
    {
        if (subCounter)
        {
            SubGroupCounter subGroupCounter(subCounter);
            counter = new NumericGroupCounter<SubGroupCounter>(prop, numericPropertyTable.get(), subGroupCounter);
        }
        else
        {
            counter = new NumericGroupCounter<>(prop, numericPropertyTable.get());
        }
    }
    else
    {
        LOG(ERROR) << "failed to create NumericPropertyTable for property " << prop;
    }

    return counter;
}

GroupCounter* GroupCounterLabelBuilder::createNumericRangeCounter_(const std::string& prop) const
{
    const GroupConfig* groupConfig = getGroupConfig_(prop);
    if (!groupConfig || !groupConfig->isNumericType())
    {
        LOG(ERROR) << "property " << prop
                   << " must be configured as numeric type for range group";
        return NULL;
    }

    boost::shared_ptr<NumericPropertyTableBase>& numericPropertyTable = numericTableBuilder_->createPropertyTable(prop);
    if (numericPropertyTable)
    {
        return new NumericRangeGroupCounter(prop, numericPropertyTable.get());
    }
    return NULL;
}

GroupCounter* GroupCounterLabelBuilder::createDateCounter_(const std::string& prop, const std::string& unit, GroupCounter* subCounter) const
{
    DATE_MASK_TYPE mask;
    std::string errorMsg;
    if (! DateStrParser::get()->unitStrToMask(unit, mask, errorMsg))
    {
        LOG(ERROR) << errorMsg;
        return NULL;
    }

    const DateGroupTable* dateTable = groupManager_->getDateGroupTable(prop);
    if (! dateTable)
    {
        LOG(ERROR) << "group index file is not loaded for group property " << prop;
        return NULL;
    }

    GroupCounter* counter = NULL;

    if (subCounter)
    {
        SubGroupCounter subGroupCounter(subCounter);
        counter = new DateGroupCounter<SubGroupCounter>(*dateTable, mask, subGroupCounter);
    }
    else
    {
        counter = new DateGroupCounter<>(*dateTable, mask);
    }

    return counter;
}

GroupLabel* GroupCounterLabelBuilder::createGroupLabel(const GroupParam::GroupLabelParam& labelParam)
{
    GroupLabel* label = NULL;

    const std::string& propName = labelParam.first;
    PropertyDataType type = getPropertyType_(propName);
    switch(type)
    {
    case STRING_PROPERTY_TYPE:
        label = createStringLabel_(labelParam);
        break;

    case INT32_PROPERTY_TYPE:
    case FLOAT_PROPERTY_TYPE:
    case INT64_PROPERTY_TYPE:
    case DOUBLE_PROPERTY_TYPE:
        label = createNumericRangeLabel_(labelParam);
        break;

    case DATETIME_PROPERTY_TYPE:
        label = createDateLabel_(labelParam);
        break;

    default:
        LOG(ERROR) << "unsupported type " << type
                   << " for group property " << propName;
        break;
    }

    return label;
}

GroupLabel* GroupCounterLabelBuilder::createStringLabel_(const GroupParam::GroupLabelParam& labelParam) const
{
    GroupLabel* label = NULL;

    const std::string& propName = labelParam.first;
    const PropValueTable* pvTable = groupManager_->getPropValueTable(propName);
    if (pvTable)
    {
        label = new StringGroupLabel(labelParam.second, *pvTable);
    }
    else
    {
        LOG(ERROR) << "group index file is not loaded for group property " << propName;
    }

    return label;
}

GroupLabel* GroupCounterLabelBuilder::createNumericRangeLabel_(const GroupParam::GroupLabelParam& labelParam) const
{
    const std::string& propName = labelParam.first;
    bool isRange = false;

    if (! checkLabelParam(labelParam, isRange))
    {
        LOG(ERROR) << "empty group label value for property " << propName;
        return NULL;
    }

    if (isRange)
        return createRangeLabel_(labelParam);

    return createNumericLabel_(labelParam);
}

GroupLabel* GroupCounterLabelBuilder::createNumericLabel_(const GroupParam::GroupLabelParam& labelParam) const
{
    const std::string& propName = labelParam.first;
    const GroupParam::GroupPathVec& paths = labelParam.second;

    const boost::shared_ptr<NumericPropertyTableBase>& numericPropertyTable = numericTableBuilder_->createPropertyTable(propName);
    if (!numericPropertyTable)
    {
        LOG(ERROR) << "failed in creating numeric table for property " << propName;
        return NULL;
    }

    std::vector<float> targetValues;
    for (GroupParam::GroupPathVec::const_iterator pathIt = paths.begin();
        pathIt != paths.end(); ++pathIt)
    {
        const GroupParam::GroupPath& path = *pathIt;
        if (path.empty())
        {
            LOG(ERROR) << "empty group label value for property " << propName;
            return NULL;
        }

        const std::string& propValue = path.front();
        float value = 0;
        if (!convertNumericLabel(propValue, value))
            return NULL;

        targetValues.push_back(value);
    }

    return new NumericRangeGroupLabel(numericPropertyTable.get(), targetValues);
}

GroupLabel* GroupCounterLabelBuilder::createRangeLabel_(const GroupParam::GroupLabelParam& labelParam) const
{
    const std::string& propName = labelParam.first;
    const GroupParam::GroupPathVec& paths = labelParam.second;

    const boost::shared_ptr<NumericPropertyTableBase>& numericPropertyTable = numericTableBuilder_->createPropertyTable(propName);
    if (!numericPropertyTable)
    {
        LOG(ERROR) << "failed in creating numeric table for property " << propName;
        return NULL;
    }

    NumericRangeGroupLabel::NumericRangeVec ranges;
    for (GroupParam::GroupPathVec::const_iterator pathIt = paths.begin();
            pathIt != paths.end(); ++pathIt)
    {
        const GroupParam::GroupPath& path = *pathIt;
        if (path.empty())
        {
            LOG(ERROR) << "empty group label value for property " << propName;
            return NULL;
        }

        const std::string& propValue = path.front();
        NumericRangeGroupLabel::NumericRange range;
        if (!convertRangeLabel(propValue, range))
            return NULL;

        ranges.push_back(range);
    }

    return new NumericRangeGroupLabel(numericPropertyTable.get(), ranges);
}

GroupLabel* GroupCounterLabelBuilder::createDateLabel_(const GroupParam::GroupLabelParam& labelParam) const
{
    const std::string& propName = labelParam.first;
    const GroupParam::GroupPathVec& labelPaths = labelParam.second;
    DateGroupLabel::DateMaskVec dateMaskVec;

    for (GroupParam::GroupPathVec::const_iterator pathIt = labelPaths.begin();
        pathIt != labelPaths.end(); ++pathIt)
    {
        const GroupParam::GroupPath& path = *pathIt;
        if (path.empty())
        {
            LOG(ERROR) << "empty group label value for property " << propName;
            return NULL;
        }

        const std::string& propValue = path.front();
        DateStrParser::DateMask dateMask;
        std::string errorMsg;
        if (! DateStrParser::get()->apiStrToDateMask(propValue, dateMask, errorMsg))
        {
            LOG(ERROR) << errorMsg;
            return NULL;
        }

        dateMaskVec.push_back(dateMask);
    }

    const DateGroupTable* dateTable = groupManager_->getDateGroupTable(propName);
    if (! dateTable)
    {
        LOG(ERROR) << "group index file is not loaded for group property " << propName;
        return NULL;
    }

    return new DateGroupLabel(*dateTable, dateMaskVec);
}

NS_FACETED_END

#include <search-manager/NumericPropertyTable.h>
#include "NumericRangeGroupLabel.h"

NS_FACETED_BEGIN

NumericRangeGroupLabel::NumericRangeGroupLabel(const NumericPropertyTable *propertyTable, const std::vector<float>& targetValues)
    : propertyTable_(propertyTable)
    , targetValueSet_(targetValues.begin(), targetValues.end())
    , test_(&NumericRangeGroupLabel::test1)
{}

NumericRangeGroupLabel::NumericRangeGroupLabel(const NumericPropertyTable *propertyTable, const NumericRangeVec& ranges)
    : propertyTable_(propertyTable)
    , ranges_(ranges)
    , test_(&NumericRangeGroupLabel::test2)
{}

NumericRangeGroupLabel::~NumericRangeGroupLabel()
{
    delete propertyTable_;
}

bool NumericRangeGroupLabel::test(docid_t doc) const
{
    return (this->*test_)(doc);
}

bool NumericRangeGroupLabel::test1(docid_t doc) const
{
    float value = 0;
    if (propertyTable_->convertPropertyValue(doc, value))
    {
        return targetValueSet_.find(value) != targetValueSet_.end();
    }

    return false;
}

bool NumericRangeGroupLabel::test2(docid_t doc) const
{
    int64_t value = 0;
    if (propertyTable_->convertPropertyValue(doc, value))
    {
        for (NumericRangeVec::const_iterator rangeIt = ranges_.begin();
            rangeIt != ranges_.end(); ++rangeIt)
        {
            if (value >= rangeIt->first && value <= rangeIt->second)
                return true;
        }
    }

    return false;
}

NS_FACETED_END

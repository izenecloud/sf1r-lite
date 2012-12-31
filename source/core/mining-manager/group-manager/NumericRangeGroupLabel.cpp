#include "NumericRangeGroupLabel.h"

NS_FACETED_BEGIN

NumericRangeGroupLabel::NumericRangeGroupLabel(const NumericPropertyTableBase *numericPropertyTable, const std::vector<float>& targetValues)
    : numericPropertyTable_(numericPropertyTable)
    , targetValueSet_(targetValues.begin(), targetValues.end())
    , test_(&NumericRangeGroupLabel::test1)
{}

NumericRangeGroupLabel::NumericRangeGroupLabel(const NumericPropertyTableBase *numericPropertyTable, const NumericRangeVec& ranges)
    : numericPropertyTable_(numericPropertyTable)
    , ranges_(ranges)
    , test_(&NumericRangeGroupLabel::test2)
{}

NumericRangeGroupLabel::~NumericRangeGroupLabel()
{
}

bool NumericRangeGroupLabel::test(docid_t doc) const
{
    return (this->*test_)(doc);
}

bool NumericRangeGroupLabel::test1(docid_t doc) const
{
    float value = 0;
    if (numericPropertyTable_->getFloatValue(doc, value, false))
    {
        return targetValueSet_.find(value) != targetValueSet_.end();
    }

    return false;
}

bool NumericRangeGroupLabel::test2(docid_t doc) const
{
    int32_t value = 0;
    if (numericPropertyTable_->getInt32Value(doc, value, false))
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

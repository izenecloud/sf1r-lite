#include <search-manager/NumericPropertyTable.h>
#include "NumericRangeGroupLabel.h"

NS_FACETED_BEGIN

NumericRangeGroupLabel::NumericRangeGroupLabel(const NumericPropertyTable *propertyTable, const float &targetValue)
    : propertyTable_(propertyTable)
    , targetValue_(targetValue)
    , test_(&NumericRangeGroupLabel::test1)
{}

NumericRangeGroupLabel::NumericRangeGroupLabel(const NumericPropertyTable *propertyTable, const int64_t &lowerBound, const int64_t &upperBound)
    : propertyTable_(propertyTable)
    , lowerBound_(lowerBound)
    , upperBound_(upperBound)
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
        return (value == targetValue_);

    return false;
}

bool NumericRangeGroupLabel::test2(docid_t doc) const
{
    int64_t value = 0;
    if (propertyTable_->convertPropertyValue(doc, value))
        return (value >= lowerBound_ && value <= upperBound_);

    return false;
}

NS_FACETED_END

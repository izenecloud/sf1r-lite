#include <search-manager/NumericPropertyTable.h>
#include "NumericRangeGroupLabel.h"

NS_FACETED_BEGIN

NumericRangeGroupLabel::NumericRangeGroupLabel(const NumericPropertyTable *propertyTable, const int64_t &lowerBound, const int64_t &upperBound)
    : propertyTable_(propertyTable)
    , lowerBound_(lowerBound)
    , upperBound_(upperBound)
{}

NumericRangeGroupLabel::~NumericRangeGroupLabel()
{
    delete propertyTable_;
}

bool NumericRangeGroupLabel::test(docid_t doc) const
{
    int64_t value;
    propertyTable_->convertPropertyValue(doc, value);
    return (value >= lowerBound_ && value <= upperBound_);
}

NS_FACETED_END

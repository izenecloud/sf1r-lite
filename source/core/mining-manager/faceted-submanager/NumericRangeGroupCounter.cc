#include <search-manager/NumericPropertyTable.h>
#include <util/ustring/UString.h>
#include <cstring>
#include <cmath>
#include "NumericRangeGroupCounter.h"

NS_FACETED_BEGIN

Log10SegmentTree::Log10SegmentTree()
{
    level0_ = 0;
    bzero(level1_, sizeof(level1_));
    bzero(level2_, sizeof(level2_));
    bzero(level3_, sizeof(level3_));
}

void Log10SegmentTree::insertPoint(int64_t point)
{
    level0_++;
    int exponent = log10(point);
    if (exponent < 2)
    {
        exponent = 2;
    }
    exponent -= 2;
    level1_[exponent]++;
    for (int i = 2; i < exponent; i++)
    {
        point /= 10;
    }
    level2_[exponent][point / 10]++;
    level3_[exponent][point / 10][point % 10]++;
}

NumericRangeGroupCounter::NumericRangeGroupCounter(const NumericPropertyTable *propertyTable, size_t rangeCount)
    : propertyTable_(propertyTable)
    , rangeCount_(rangeCount)
    , log10SegmentTree_()
{}

NumericRangeGroupCounter::~NumericRangeGroupCounter()
{
    delete propertyTable_;
}

void NumericRangeGroupCounter::addDoc(docid_t doc)
{
    int64_t value;
    propertyTable_->getPropertyValue(doc, value);
    log10SegmentTree_.insertPoint(value);
}

void NumericRangeGroupCounter::getGroupRep(OntologyRep& groupRep) const
{
    std::list<OntologyRepItem>& itemList = groupRep.item_list;

    izenelib::util::UString propName(propertyTable_->getPropertyName(), UString::UTF_8);
    size_t count = log10SegmentTree_.level0_;
    itemList.push_back(faceted::OntologyRepItem(0, propName, 0, count));

//  TODO: design a strategy to split into several ranges

}

NS_FACETED_END

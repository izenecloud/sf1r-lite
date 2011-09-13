#include <search-manager/NumericPropertyTable.h>
#include <util/ustring/UString.h>
#include <cstring>
#include <cmath>
#include "NumericRangeGroupCounter.h"

NS_FACETED_BEGIN

const int Log10SegmentTree::span_[8] = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000};

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
    if (exponent < 1)
    {
        exponent = 1;
    }
    exponent--;
    ++level1_[exponent];
    for (int i = 0; i < exponent; i++)
    {
        point /= 10;
    }
    ++level2_[exponent][point / 10];
    ++level3_[exponent][point / 10][point % 10];
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
    propertyTable_->convertPropertyValue(doc, value);
    log10SegmentTree_.insertPoint(value);
}

void NumericRangeGroupCounter::getGroupRep(OntologyRep& groupRep) const
{
    std::list<OntologyRepItem>& itemList = groupRep.item_list;

    izenelib::util::UString propName(propertyTable_->getPropertyName(), UString::UTF_8);
    size_t count = log10SegmentTree_.level0_;
    itemList.push_back(faceted::OntologyRepItem(0, propName, 0, count));

    for (int i = 0; i < 8; i++)
    {
        if (log10SegmentTree_.level1_[i] == 0)
            continue;

        itemList.push_back(faceted::OntologyRepItem());
        faceted::OntologyRepItem& repItem = itemList.back();
        repItem.level = 1;
        std::stringstream ss;
        int lowerBound = exp10(i + 1);
        if (lowerBound == 10)
            lowerBound = 0;

        ss << lowerBound << "-" << (int) exp10(i + 2) - 1;
        izenelib::util::UString stringValue(ss.str(), UString::UTF_8);
        repItem.text = stringValue;
        repItem.doc_count = log10SegmentTree_.level1_[i];
    }
}

NS_FACETED_END

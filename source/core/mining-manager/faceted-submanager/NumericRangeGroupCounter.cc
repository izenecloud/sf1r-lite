#include <search-manager/NumericPropertyTable.h>
#include <util/ustring/UString.h>
#include <cstring>
#include "NumericRangeGroupCounter.h"

NS_FACETED_BEGIN

const int Log10SegmentTree::bound_[9]
    = {0, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000};

Log10SegmentTree::Log10SegmentTree()
{
    level0_ = 0;
    bzero(level1_, sizeof(level1_));
    bzero(level2_, sizeof(level2_));
    bzero(level3_, sizeof(level3_));
}

void Log10SegmentTree::insertPoint(int64_t point)
{
    ++level0_;
    int exponent;
    for (exponent = 0; point >= 100; ++exponent)
        point /= 10;

    ++level1_[exponent];
    ++level2_[exponent][point / 10];
    ++level3_[exponent][point / 10][point % 10];
}

NumericRangeGroupCounter::NumericRangeGroupCounter(const NumericPropertyTable *propertyTable, size_t rangeCount)
    : propertyTable_(propertyTable)
    , rangeCount_(rangeCount)
    , segmentTree_()
{}

NumericRangeGroupCounter::~NumericRangeGroupCounter()
{
    delete propertyTable_;
}

void NumericRangeGroupCounter::addDoc(docid_t doc)
{
    int64_t value;
    propertyTable_->convertPropertyValue(doc, value);
    segmentTree_.insertPoint(value);
}

void NumericRangeGroupCounter::getGroupRep(OntologyRep& groupRep) const
{
    std::list<OntologyRepItem>& itemList = groupRep.item_list;

    izenelib::util::UString propName(propertyTable_->getPropertyName(), UString::UTF_8);
    size_t count = segmentTree_.level0_;
    itemList.push_back(faceted::OntologyRepItem(0, propName, 0, count));

    get_range_list_drop_blanks(itemList);
}

void NumericRangeGroupCounter::get_range_list_raw(std::list<OntologyRepItem>& itemList) const
{
    for (int i = 0; i < 8; i++)
    {
        if (segmentTree_.level1_[i] == 0)
            continue;

        itemList.push_back(faceted::OntologyRepItem());
        faceted::OntologyRepItem& repItem = itemList.back();
        repItem.level = 1;
        std::stringstream ss;

        ss << Log10SegmentTree::bound_[i] << "-" << Log10SegmentTree::bound_[i + 1] - 1;
        izenelib::util::UString stringValue(ss.str(), UString::UTF_8);
        repItem.text = stringValue;
        repItem.doc_count = segmentTree_.level1_[i];
    }
}

void NumericRangeGroupCounter::get_range_list_drop_blanks(std::list<OntologyRepItem>& itemList) const
{
    for (int i = 0; i < 8; i++)
    {
        if (segmentTree_.level1_[i] == 0)
            continue;

        itemList.push_back(faceted::OntologyRepItem());
        faceted::OntologyRepItem& repItem = itemList.back();
        repItem.level = 1;
        std::stringstream ss;
        std::pair<int, int> begin(0, 0), end(9, 9);
        while (segmentTree_.level2_[i][begin.first] == 0)
            ++begin.first;

        while (segmentTree_.level3_[i][begin.first][begin.second] == 0)
            ++begin.second;

        while (segmentTree_.level2_[i][end.first] == 0)
            --end.first;

        while (segmentTree_.level3_[i][end.first][end.second] == 0)
            --end.second;

        int atom = segmentTree_.bound_[i + 1] / 100;
        ss << atom * (10 * begin.first + begin.second)
           << "-"
           << atom * (10 * end.first + end.second + 1) - 1;

        izenelib::util::UString stringValue(ss.str(), UString::UTF_8);
        repItem.text = stringValue;
        repItem.doc_count = segmentTree_.level1_[i];
    }
}

void NumericRangeGroupCounter::get_range_list_split(std::list<OntologyRepItem>& itemList) const
{
//  TODO
}

void NumericRangeGroupCounter::get_range_list_split_and_merge(std::list<OntologyRepItem>& itemList) const
{
//  TODO
}

NS_FACETED_END

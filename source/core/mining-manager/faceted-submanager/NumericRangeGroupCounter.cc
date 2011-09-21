#include <search-manager/NumericPropertyTable.h>
#include <util/ustring/UString.h>
#include <cstring>
#include "NumericRangeGroupCounter.h"

NS_FACETED_BEGIN

const int Log10SegmentTree::bound_[]
    = {0, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000};

Log10SegmentTree::Log10SegmentTree()
{
    level0_ = 0;
    bzero(level1_, sizeof(level1_));
    bzero(level2_, sizeof(level2_));
    bzero(level3_, sizeof(level3_));
}

void Log10SegmentTree::insertPoint(int point)
{
    ++level0_;
    int exponent;
    for (exponent = 1; point >= bound_[exponent]; ++exponent);
    point /= bound_[exponent--] / 100;
    ++level1_[exponent];
    ++level2_[exponent][point / 10];
    ++level3_[exponent][point / 10][point % 10];
}

NumericRangeGroupCounter::NumericRangeGroupCounter(const NumericPropertyTable *propertyTable, RangePolicy rangePolicy, int maxRangeNumber)
    : propertyTable_(propertyTable)
    , rangePolicy_(rangePolicy)
    , maxRangeNumber_(maxRangeNumber)
    , segmentTree_()
{}

NumericRangeGroupCounter::~NumericRangeGroupCounter()
{
    delete propertyTable_;
}

void NumericRangeGroupCounter::addDoc(docid_t doc)
{
    int value;
    if (propertyTable_->convertPropertyValue(doc, value))
        segmentTree_.insertPoint(value);
}

void NumericRangeGroupCounter::getGroupRep(OntologyRep& groupRep) const
{
    std::list<OntologyRepItem>& itemList = groupRep.item_list;

    izenelib::util::UString propName(propertyTable_->getPropertyName(), UString::UTF_8);
    int count = segmentTree_.level0_;
    itemList.push_back(faceted::OntologyRepItem(0, propName, 0, count));

    switch (rangePolicy_)
    {
    case RAW:
        get_range_list_raw(itemList);
        break;

    case DROP_BLANKS:
        get_range_list_drop_blanks(itemList);
        break;

    case SPLIT:
        get_range_list_split(itemList);
        break;

    case SPLIT_AND_MERGE:
        get_range_list_split_and_merge(itemList);
        break;

    default:
        break;
    }
}

void NumericRangeGroupCounter::get_range_list_raw(std::list<OntologyRepItem>& itemList) const
{
    for (int i = 0; i < LEVEL_1_OF_SEGMENT_TREE; i++)
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
    for (int i = 0; i < LEVEL_1_OF_SEGMENT_TREE; i++)
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
            << atom * (10 * end.first + end.second + 1);

        izenelib::util::UString stringValue(ss.str(), UString::UTF_8);
        repItem.text = stringValue;
        repItem.doc_count = segmentTree_.level1_[i];
    }
}

void NumericRangeGroupCounter::get_range_list_split(std::list<OntologyRepItem>& itemList) const
{
    int split[8];
    int rangeNumber = maxRangeNumber_;
    for (int i = 0; i < LEVEL_1_OF_SEGMENT_TREE; i++)
    {
        if (segmentTree_.level1_[i])
        {
            split[i] = 1;
            --rangeNumber;
        }
        else
            split[i] = 0;
    }
    while (rangeNumber > 0)
    {
        int best = -1;
        double bestVarianceReduce;
        for (int i = 0; i < LEVEL_1_OF_SEGMENT_TREE; i++)
        {
            if (segmentTree_.level1_[i] <= split[i])
                continue;

            double varianceReduce = (double) segmentTree_.level1_[i] * segmentTree_.level1_[i] / split[i] / (split[i] + 1);
            if (best == -1 || varianceReduce > bestVarianceReduce)
            {
                best = i;
                bestVarianceReduce = varianceReduce;
            }
        }
        if (best != -1)
        {
            ++split[best];
            --rangeNumber;
        }
        else break;
    }
    for (int i = 0; i < LEVEL_1_OF_SEGMENT_TREE; i++)
    {
        if (!split[i])
            continue;

        std::pair<int, int> stop(0, 0), end(9, 9);
        int tempCount = 0, oldCount = 0;
        int atom = segmentTree_.bound_[i + 1] / 100;
        while (segmentTree_.level2_[i][stop.first] == 0)
            ++stop.first;

        while (segmentTree_.level3_[i][stop.first][stop.second] == 0)
            ++stop.second;

        while (segmentTree_.level2_[i][end.first] == 0)
            --end.first;

        while (segmentTree_.level3_[i][end.first][end.second] == 0)
            --end.second;

        for (int j = 0; j < split[i] - 1; j++)
        {
            int difference = tempCount - ((j + 1) * segmentTree_.level1_[i] - 1) / split[i] - 1;
            std::pair<int, int> tempStop, oldStop(stop);
            while (true)
            {
                int newDifference = difference + segmentTree_.level3_[i][stop.first][stop.second];
                if (newDifference + difference >= 0)
                    break;

                if (newDifference != difference)
                {
                    tempCount += segmentTree_.level3_[i][stop.first][stop.second];
                    difference = newDifference;
                    tempStop = stop;
                }
                if (tempCount == segmentTree_.level1_[i])
                    break;

                while (++stop.second < 10 && segmentTree_.level3_[i][stop.first][stop.second] == 0);
                if (stop.second == 10)
                {
                    while (segmentTree_.level2_[i][++stop.first] == 0);
                    stop.second = 0;
                    while (segmentTree_.level3_[i][stop.first][stop.second] == 0)
                        ++stop.second;
                }
            }
            if (oldCount != tempCount)
            {
                itemList.push_back(faceted::OntologyRepItem());
                faceted::OntologyRepItem& repItem = itemList.back();
                repItem.level = 1;
                std::stringstream ss;
                ss << atom * (10 * oldStop.first + oldStop.second)
                    << "-"
                    << atom * (10 * tempStop.first + tempStop.second + 1) - 1;

                izenelib::util::UString stringValue(ss.str(), UString::UTF_8);
                repItem.text = stringValue;
                repItem.doc_count = tempCount - oldCount;
                oldCount = tempCount;
            }
        }
        if (tempCount == segmentTree_.level1_[i])
            continue;

        itemList.push_back(faceted::OntologyRepItem());
        faceted::OntologyRepItem& repItem = itemList.back();
        repItem.level = 1;
        std::stringstream ss;
        ss << atom * (10 * stop.first + stop.second)
            << "-"
            << atom * (10 * end.first + end.second + 1) - 1;

        izenelib::util::UString stringValue(ss.str(), UString::UTF_8);
        repItem.text = stringValue;
        repItem.doc_count = segmentTree_.level1_[i] - tempCount;
    }
}

void NumericRangeGroupCounter::get_range_list_split_and_merge(std::list<OntologyRepItem>& itemList) const
{
//  May not need this one any more
}

NS_FACETED_END

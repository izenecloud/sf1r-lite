#include "NumericRangeGroupCounter.h"

#include <util/ustring/UString.h>
#include "GroupRep.h"

NS_FACETED_BEGIN

const int NumericRangeGroupCounter::bound_[]
    = {0, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000};

NumericRangeGroupCounter::NumericRangeGroupCounter(const std::string& property, const NumericPropertyTableBase* numericPropertyTable)
    : property_(property)
    , numericPropertyTable_(numericPropertyTable)
    , segmentTree_(111 * LEVEL_1_OF_SEGMENT_TREE + 1)
{
}

NumericRangeGroupCounter::~NumericRangeGroupCounter()
{
}

NumericRangeGroupCounter* NumericRangeGroupCounter::clone() const
{
    return new NumericRangeGroupCounter(*this);
}

void NumericRangeGroupCounter::addDoc(docid_t doc)
{
    int32_t value;
    if (numericPropertyTable_->getInt32Value(doc, value, false) && value > 0)
    {
        value = std::min(value, bound_[LEVEL_1_OF_SEGMENT_TREE] - 1);
        int exponent;
        for (exponent = 1; value >= bound_[exponent]; ++exponent);
        value /= bound_[exponent--] / 100;
        ++segmentTree_[100 * exponent + value];
    }
}

void NumericRangeGroupCounter::getGroupRep(GroupRep &groupRep)
{
    vector<unsigned int>::const_iterator it = segmentTree_.begin();
    vector<unsigned int>::iterator it0 = segmentTree_.begin() + 100 * LEVEL_1_OF_SEGMENT_TREE;

    for (int i = 0; i < LEVEL_1_OF_SEGMENT_TREE * 11; i++)
    {
        for (int j = 0; j < 10; j++)
            *it0 += *(it++);

        it0++;
    }
    for (int i = 0; i < LEVEL_1_OF_SEGMENT_TREE; i++)
        *it0 += *(it++);

    groupRep.numericRangeGroupRep_.push_back(make_pair(property_, vector<unsigned int>()));
    segmentTree_.swap(groupRep.numericRangeGroupRep_.back().second);
}

void NumericRangeGroupCounter::toOntologyRepItemList(GroupRep &groupRep)
{
    GroupRep::StringGroupRep& itemList = groupRep.stringGroupRep_;

    for (GroupRep::NumericRangeGroupRep::const_iterator it = groupRep.numericRangeGroupRep_.begin();
        it != groupRep.numericRangeGroupRep_.end(); ++it)
    {
        izenelib::util::UString propName(it->first, izenelib::util::UString::UTF_8);
        itemList.push_back(faceted::OntologyRepItem(0, propName, 0, it->second.back()));

        vector<unsigned int>::const_iterator level3 = it->second.begin();
        vector<unsigned int>::const_iterator level2 = level3 + 100 * LEVEL_1_OF_SEGMENT_TREE;
        vector<unsigned int>::const_iterator level1 = level2 + 10 * LEVEL_1_OF_SEGMENT_TREE;
        unsigned int split[LEVEL_1_OF_SEGMENT_TREE];
        int rangeNumber = MAX_RANGE_NUMBER;

        vector<unsigned int>::const_iterator vit = level3;

        for (int i = 0; i < LEVEL_1_OF_SEGMENT_TREE; i++)
        {
            if (level1[i])
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
            double bestVarianceReduce = 0;

            for (int i = 0; i < LEVEL_1_OF_SEGMENT_TREE; i++)
            {
                if (level1[i] <= split[i])
                    continue;

                double varianceReduce = (double) level1[i] * level1[i] / split[i] / (split[i] + 1);

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

        for (unsigned int i = 0; i < LEVEL_1_OF_SEGMENT_TREE; i++)
        {
            if (!split[i])
            {
                level3 += 100;
                level2 += 10;
                level1 ++;
                continue;
            }

            pair<int, int> stop(0, 0), end(9, 9);
            unsigned int tempCount = 0, oldCount = 0;
            int atom = bound_[i + 1] / 100;

            while (level2[end.first] == 0)
                --end.first;

            vit = level3 + 10 * end.first + 9;
            while (*vit == 0)
            {
                --end.second;
                --vit;
            }

            while (level2[stop.first] == 0)
                ++stop.first;

            vit = level3 + 10 * stop.first;
            while (*vit == 0)
            {
                ++stop.second;
                ++vit;
            }

            for (unsigned int j = 0; j < split[i] - 1; j++)
            {
                int difference = tempCount - ((j + 1) * (*level1) - 1) / split[i] - 1;
                pair<int, int> tempStop, oldStop(stop);

                while (true)
                {
                    int newDifference = difference + *vit;
                    if (newDifference + difference >= 0)
                        break;

                    if (newDifference != difference)
                    {
                        tempCount += *vit;
                        difference = newDifference;
                        tempStop = stop;
                    }
                    if (tempCount == *level1)
                        break;

                    while (++stop.second < 10 && *(++vit) == 0);
                    if (stop.second == 10)
                    {
                        while (level2[++stop.first] == 0);
                        stop.second = 0;
                        vit = level3 + 10 * stop.first;
                        while (*vit == 0)
                        {
                            ++stop.second;
                            ++vit;
                        }
                    }
                }

                if (oldCount != tempCount)
                {
                    stringstream ss;
                    ss << atom * (10 * oldStop.first + oldStop.second)
                        << "-"
                        << atom * (10 * tempStop.first + tempStop.second + 1) - 1;
                    izenelib::util::UString ustr(ss.str(), izenelib::util::UString::UTF_8);
                    itemList.push_back(faceted::OntologyRepItem(1, ustr, 0, tempCount - oldCount));
                    if (tempCount == *level1)
                        break;

                    oldCount = tempCount;
                }
            }

            if (tempCount != *level1)
            {
                stringstream ss;
                ss << atom * (10 * stop.first + stop.second)
                    << "-"
                    << atom * (10 * end.first + end.second + 1) - 1;
                izenelib::util::UString ustr(ss.str(), izenelib::util::UString::UTF_8);
                itemList.push_back(faceted::OntologyRepItem(1, ustr, 0, *level1 - tempCount));
            }

            level3 += 100;
            level2 += 10;
            level1 ++;
        }
    }
}

NS_FACETED_END

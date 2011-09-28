#include <search-manager/NumericPropertyTable.h>
#include <util/ustring/UString.h>
#include <cstring>
#include "NumericRangeGroupCounter.h"

NS_FACETED_BEGIN

const int NumericRangeGroupCounter::bound_[]
    = {0, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000};

NumericRangeGroupCounter::NumericRangeGroupCounter(const NumericPropertyTable *propertyTable)
    : propertyTable_(propertyTable)
{
    segmentTree_.resize(100 * LEVEL_1_OF_SEGMENT_TREE);
}

NumericRangeGroupCounter::~NumericRangeGroupCounter()
{
    delete propertyTable_;
}

void NumericRangeGroupCounter::addDoc(docid_t doc)
{
    int value;
    if (propertyTable_->convertPropertyValue(doc, value))
    {
        int exponent;
        for (exponent = 1; value >= bound_[exponent]; ++exponent);
        value /= bound_[exponent--] / 100;
        ++segmentTree_[100 * exponent + value];
    }
}

void NumericRangeGroupCounter::getGroupRep(GroupRep &groupRep)
{
    groupRep.numericRangeGroupRep_.push_back(make_pair(propertyTable_->getPropertyName(), std::vector<unsigned int>()));
    segmentTree_.swap(groupRep.numericRangeGroupRep_.back().second);
}

void NumericRangeGroupCounter::toOntologyRepItemList(GroupRep &groupRep)
{
    GroupRep::StringGroupRep& itemList = groupRep.stringGroupRep_;

    for (GroupRep::NumericRangeGroupRep::const_iterator it = groupRep.numericRangeGroupRep_.begin();
        it != groupRep.numericRangeGroupRep_.end(); ++it)
    {
        unsigned int level0 = 0;
        unsigned int level1[LEVEL_1_OF_SEGMENT_TREE];
        unsigned int level2[LEVEL_1_OF_SEGMENT_TREE][10];
        const vector<unsigned int> &level3 = it->second;
        unsigned int split[LEVEL_1_OF_SEGMENT_TREE];
        int rangeNumber = MAX_RANGE_NUMBER;

        std::vector<unsigned int>::const_iterator vit = level3.begin();

        for (int i = 0; i < LEVEL_1_OF_SEGMENT_TREE; i++)
        {
            level1[i] = 0;
            for (int j = 0; j < 10; j++)
            {
                level2[i][j] = 0;
                for (int k = 0; k < 10; k++)
                {
                    level2[i][j] += *(vit++);
                }
                level1[i] += level2[i][j];
            }
            level0 += level1[i];
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
            double bestVarianceReduce;

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

        izenelib::util::UString propName(it->first, UString::UTF_8);
        itemList.push_back(faceted::OntologyRepItem(0, propName, 0, level0));

        for (unsigned int i = 0; i < LEVEL_1_OF_SEGMENT_TREE; i++)
        {
            std::vector<unsigned int>::const_iterator bit = level3.begin() + 100 * i;
            if (!split[i])
                continue;

            std::pair<int, int> stop(0, 0), end(9, 9);
            unsigned int tempCount = 0, oldCount = 0;
            int atom = bound_[i + 1] / 100;

            while (level2[i][end.first] == 0)
                --end.first;

            vit = bit + 10 * end.first + 9;
            while (*vit == 0)
            {
                --end.second;
                --vit;
            }

            while (level2[i][stop.first] == 0)
                ++stop.first;

            vit = bit + 10 * stop.first;
            while (*vit == 0)
            {
                ++stop.second;
                ++vit;
            }

            for (unsigned int j = 0; j < split[i] - 1; j++)
            {
                int difference = tempCount - ((j + 1) * level1[i] - 1) / split[i] - 1;
                std::pair<int, int> tempStop, oldStop(stop);

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
                    if (tempCount == level1[i])
                        break;

                    while (++stop.second < 10 && *(++vit) == 0);
                    if (stop.second == 10)
                    {
                        while (level2[i][++stop.first] == 0);
                        stop.second = 0;
                        vit = bit + 10 * stop.first;
                        while (*vit == 0)
                        {
                            ++stop.second;
                            ++vit;
                        }
                    }
                }

                if (oldCount != tempCount)
                {
                    std::stringstream ss;
                    ss << atom * (10 * oldStop.first + oldStop.second)
                        << "-"
                        << atom * (10 * tempStop.first + tempStop.second + 1) - 1;
                    izenelib::util::UString ustr(ss.str(), UString::UTF_8);
                    itemList.push_back(faceted::OntologyRepItem(1, ustr, 0, tempCount - oldCount));
                    if (tempCount == level1[i])
                        break;

                    oldCount = tempCount;
                }
            }

            if (tempCount == level1[i])
                continue;

            std::stringstream ss;
            ss << atom * (10 * stop.first + stop.second)
                << "-"
                << atom * (10 * end.first + end.second + 1) - 1;
            izenelib::util::UString ustr(ss.str(), UString::UTF_8);
            itemList.push_back(faceted::OntologyRepItem(1, ustr, 0, level1[i] - tempCount));
        }
    }
    groupRep.numericRangeGroupRep_.clear();
}

NS_FACETED_END

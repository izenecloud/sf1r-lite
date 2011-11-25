#include "LabelCounter.h"

#include <util/PriorityQueue.h>

#include <glog/logging.h>

namespace
{

/** value => freq */
typedef std::pair<sf1r::LabelCounter::value_type, int> ValueFreq;

class ValueFreqQueue : public izenelib::util::PriorityQueue<ValueFreq>
{
public:
    ValueFreqQueue(size_t size)
    {
        this->initialize(size);
    }

protected:
    bool lessThan(const ValueFreq& p1, const ValueFreq& p2) const
    {
        return (p1.second < p2.second);
    }
};

}

namespace sf1r
{

LabelCounter::LabelCounter()
    : topValue_(0)
    , topFreq_(0)
{
}

void LabelCounter::increment(faceted::PropValueTable::pvid_t pvId)
{
    int newFreq = ++valueFreqMap_[pvId];

    if (isSetTopLabel_() == false)
    {
        if (newFreq > topFreq_)
        {
            topValue_ = pvId;
            topFreq_ = newFreq;
        }
    }
}

void LabelCounter::setTopLabel(value_type value)
{
    if (value)
    {
        topValue_ = value;
        topFreq_ = -1;
    }
    else
    {
        if (isSetTopLabel_())
        {
            topValue_ = 0;
            topFreq_ = 0;

            // reset to top freq
            for (ValueFreqMap::const_iterator it = valueFreqMap_.begin();
                it != valueFreqMap_.end(); ++it)
            {
                if (it->second > topFreq_)
                {
                    topValue_ = it->first;
                    topFreq_ = it->second;
                }
            }
        }
    }
}

void LabelCounter::getFreqLabel(
    int limit,
    std::vector<value_type>& valueVec,
    std::vector<int>& freqVec
) const
{
    if (limit < 1)
        return;

    if (limit == 1)
    {
        if (topValue_)
        {
            int freq = topFreq_;

            if (isSetTopLabel_())
            {
                ValueFreqMap::const_iterator it = valueFreqMap_.find(topValue_);
                freq = (it != valueFreqMap_.end()) ? it->second: 0;
            }

            valueVec.push_back(topValue_);
            freqVec.push_back(freq);
        }
        return;
    }

    // is filter topValue_
    bool isFilter = false;
    value_type filterValue;
    int filterFreq = 0;
    if (isSetTopLabel_())
    {
        isFilter = true;
        filterValue = topValue_;
        --limit;
    }

    // get topK results
    ValueFreqQueue queue(limit);
    for (ValueFreqMap::const_iterator mapIt = valueFreqMap_.begin();
            mapIt != valueFreqMap_.end(); ++mapIt)
    {
        if (isFilter && mapIt->first == filterValue)
        {
            filterFreq = mapIt->second;
        }
        else
        {
            queue.insert(*mapIt);
        }
    }

    const std::size_t queueSize = queue.size();
    valueVec.resize(queueSize);
    freqVec.resize(queueSize);

    std::vector<value_type>::reverse_iterator ritValue = valueVec.rbegin();
    std::vector<int>::reverse_iterator ritFreq = freqVec.rbegin();
    for(; ritValue != valueVec.rend(); ++ritValue, ++ritFreq)
    {
        ValueFreq valueFreq = queue.pop();
        *ritValue = valueFreq.first;
        *ritFreq = valueFreq.second;
    }

    if (isFilter)
    {
        valueVec.insert(valueVec.begin(), filterValue);
        freqVec.insert(freqVec.begin(), filterFreq);
    }
}

}

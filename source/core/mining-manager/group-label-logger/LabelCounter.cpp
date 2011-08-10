#include "LabelCounter.h"

#include <util/PriorityQueue.h>

#include <glog/logging.h>

namespace
{

/** value => freq */
typedef std::pair<std::string, int> ValueFreq;

class ValueFreqQueue : public izenelib::util::PriorityQueue<ValueFreq>
{
public:
    ValueFreqQueue(size_t size)
    {
        this->initialize(size);
    }

protected:
    bool lessThan(ValueFreq p1, ValueFreq p2)
    {
        return (p1.second < p2.second);
    }
};

}

namespace sf1r
{

LabelCounter::LabelCounter()
    : topFreq_(0)
{
}

void LabelCounter::increment(const std::string& propValue)
{
    int newFreq = ++valueFreqMap_[propValue];

    if (isSetTopLabel_() == false)
    {
        if (newFreq > topFreq_)
        {
            topValue_ = propValue;
            topFreq_ = newFreq;
        }
    }
}

void LabelCounter::setTopLabel(const std::string& propValue)
{
    if (propValue.empty())
    {
        // reset to top freq
        if (isSetTopLabel_())
        {
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
    else
    {
        topValue_ = propValue;
        topFreq_ = -1;
    }
}

void LabelCounter::getFreqLabel(
    int limit,
    std::vector<std::string>& propValueVec,
    std::vector<int>& freqVec
) const
{
    if (limit < 1)
        return;

    if (limit == 1)
    {
        if (! topValue_.empty())
        {
            int freq = topFreq_;

            if (isSetTopLabel_())
            {
                ValueFreqMap::const_iterator it = valueFreqMap_.find(topValue_);
                freq = (it != valueFreqMap_.end()) ? it->second: 0;
            }

            propValueVec.push_back(topValue_);
            freqVec.push_back(freq);
        }
        return;
    }

    // is filter topValue_
    bool isFilter = false;
    std::string filterStr;
    int filterFreq = 0;
    if (isSetTopLabel_())
    {
        isFilter = true;
        filterStr = topValue_;
        --limit;
    }

    // get topK results
    ValueFreqQueue queue(limit);
    for (ValueFreqMap::const_iterator mapIt = valueFreqMap_.begin();
            mapIt != valueFreqMap_.end(); ++mapIt)
    {
        if (isFilter && mapIt->first == filterStr)
        {
            filterFreq = mapIt->second;
        }
        else
        {
            queue.insert(*mapIt);
        }
    }

    const std::size_t queueSize = queue.size();
    propValueVec.resize(queueSize);
    freqVec.resize(queueSize);

    std::vector<std::string>::reverse_iterator ritValue = propValueVec.rbegin();
    std::vector<int>::reverse_iterator ritFreq = freqVec.rbegin();
    for(; ritValue != propValueVec.rend(); ++ritValue, ++ritFreq)
    {
        ValueFreq valueFreq = queue.pop();
        *ritValue = valueFreq.first;
        *ritFreq = valueFreq.second;
    }

    if (isFilter)
    {
        propValueVec.insert(propValueVec.begin(), filterStr);
        freqVec.insert(freqVec.begin(), filterFreq);
    }
}

}

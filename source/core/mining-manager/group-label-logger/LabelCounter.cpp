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

    if (propValue == topValue_)
    {
        topFreq_ = newFreq;
    }
    else if (newFreq > topFreq_)
    {
        topValue_ = propValue;
        topFreq_ = newFreq;
    }

    DLOG(INFO) << "propValue: " << propValue << ", newFreq: " << newFreq;
}

void LabelCounter::getFreqLabel(
    int limit,
    std::vector<std::string>& propValueVec,
    std::vector<int>& freqVec
)
{
    if (limit < 1)
        return;

    if (limit == 1)
    {
        if (! topValue_.empty())
        {
            propValueVec.push_back(topValue_);
            freqVec.push_back(topFreq_);

            DLOG(INFO) << "topValue_: " << topValue_ << ", topFreq_: " << topFreq_;
        }
        return;
    }

    // get topK results
    ValueFreqQueue queue(limit);
    for (ValueFreqMap::const_iterator mapIt = valueFreqMap_.begin();
            mapIt != valueFreqMap_.end(); ++mapIt)
    {
        queue.insert(*mapIt);
    }

    propValueVec.resize(queue.size());
    freqVec.resize(queue.size());

    std::vector<std::string>::reverse_iterator ritValue = propValueVec.rbegin();
    std::vector<int>::reverse_iterator ritFreq = freqVec.rbegin();
    for(; ritValue != propValueVec.rend(); ++ritValue, ++ritFreq)
    {
        ValueFreq valueFreq = queue.pop();
        *ritValue = valueFreq.first;
        *ritFreq = valueFreq.second;
    }
}

}

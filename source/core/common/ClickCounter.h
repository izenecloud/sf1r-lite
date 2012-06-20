///
/// @file ClickCounter.h
/// @brief count the frequency of clicks
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2012-03-15
///

#ifndef SF1R_CLICK_COUNTER_H
#define SF1R_CLICK_COUNTER_H

#include <vector>
#include <map>
#include <util/PriorityQueue.h>

#include <boost/serialization/access.hpp>

namespace sf1r
{

template <typename ValueType,
          typename FreqType>
class ClickCounter
{
public:
    typedef ValueType value_type;
    typedef FreqType freq_type;

    ClickCounter();

    /**
     * It would increment the counter for @p value by 1.
     * @param value the value to increment
     */
    void click(value_type value);

    /**
     * Get the most frequently clicked values.
     * @param[in] limit the max number of values to get
     * @param[out] valueVec store the values
     * @param[out] freqVec store the click count for each value in @p valueVec
     * @post @p freqVec is sorted in descending order.
     */
    void getFreqClick(
        int limit,
        std::vector<value_type>& valueVec,
        std::vector<freq_type>& freqVec
    ) const;

    /**
     * Get how many times @c click() is called for all values.
     * @return total count
     */
    freq_type getTotalFreq() const { return totalFreq_; }

    template <class Archive>
    void serialize(Archive& ar, const unsigned int version);

private:
    typedef std::map<value_type, freq_type> ValueFreqMap;
    ValueFreqMap valueFreqMap_;

    freq_type totalFreq_;

    typedef std::pair<value_type, freq_type> ValueFreq;
    class ValueFreqQueue;
};

template <typename ValueType, typename FreqType>
ClickCounter<ValueType, FreqType>::ClickCounter()
    : totalFreq_(0)
{
}

template <typename ValueType, typename FreqType>
void ClickCounter<ValueType, FreqType>::click(value_type value)
{
    ++valueFreqMap_[value];
    ++totalFreq_;
}

template <typename ValueType, typename FreqType>
void ClickCounter<ValueType, FreqType>::getFreqClick(
    int limit,
    std::vector<value_type>& valueVec,
    std::vector<freq_type>& freqVec
) const
{
    if (limit < 1)
        return;

    ValueFreqQueue queue(limit);
    for (typename ValueFreqMap::const_iterator mapIt = valueFreqMap_.begin();
        mapIt != valueFreqMap_.end(); ++mapIt)
    {
        queue.insert(*mapIt);
    }

    const std::size_t queueSize = queue.size();
    valueVec.resize(queueSize);
    freqVec.resize(queueSize);

    typename std::vector<value_type>::reverse_iterator ritValue = valueVec.rbegin();
    typename std::vector<freq_type>::reverse_iterator ritFreq = freqVec.rbegin();
    for(; ritValue != valueVec.rend(); ++ritValue, ++ritFreq)
    {
        ValueFreq valueFreq = queue.pop();
        *ritValue = valueFreq.first;
        *ritFreq = valueFreq.second;
    }
}

template <typename ValueType, typename FreqType>
template <class Archive>
void ClickCounter<ValueType, FreqType>::serialize(Archive& ar, const unsigned int version)
{
    ar & valueFreqMap_;
    ar & totalFreq_;
}

template <typename ValueType, typename FreqType>
class ClickCounter<ValueType, FreqType>::ValueFreqQueue
    : public izenelib::util::PriorityQueue<typename ClickCounter::ValueFreq>
{
public:
    ValueFreqQueue(size_t size) { this->initialize(size); }

protected:
    bool lessThan(const ValueFreq& p1, const ValueFreq& p2) const
    {
        return (p1.second < p2.second);
    }
};

} // namespace sf1r

#endif // SF1R_CLICK_COUNTER_H

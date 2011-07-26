///
/// @file LabelCounter.h
/// @brief count the frequency for each property value
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2011-07-25
///

#ifndef SF1R_LABEL_COUNTER
#define SF1R_LABEL_COUNTER

#include <string>
#include <vector>
#include <map>

#include <boost/serialization/access.hpp>

namespace sf1r
{

class LabelCounter {
public:
    LabelCounter();

    /**
     * Increment the count for @p propValue
     * @param propValue the property value
     */
    void increment(const std::string& propValue);

    /**
     * Get the most frequently clicked group labels.
     * @param limit the max number of labels to get
     * @param propValueVec store the property values of each group label
     * @param freqVec the click count for each group label
     * @post @p freqVec is sorted in descending order.
     */
    void getFreqLabel(
        int limit,
        std::vector<std::string>& propValueVec,
        std::vector<int>& freqVec
    );

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & valueFreqMap_;
        ar & topValue_;
        ar & topFreq_;
    }

private:
    /** property value => click frequency */
    typedef std::map<std::string, int> ValueFreqMap;
    ValueFreqMap valueFreqMap_;

    /** the property value with highest frequency */
    std::string topValue_;

    /** the highest frequency */
    int topFreq_;
};

}

#endif 

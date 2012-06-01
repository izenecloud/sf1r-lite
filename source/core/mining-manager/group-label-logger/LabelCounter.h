///
/// @file LabelCounter.h
/// @brief count the frequency for each property value
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2011-07-25
///

#ifndef SF1R_LABEL_COUNTER
#define SF1R_LABEL_COUNTER

#include <mining-manager/group-manager/PropValueTable.h> // pvid_t

#include <vector>
#include <map>

#include <boost/serialization/access.hpp>

namespace sf1r
{

class LabelCounter {
public:
    LabelCounter();

    /** the value type to count */
    typedef faceted::PropValueTable::pvid_t value_type;

    /**
     * Increment the count for @p value
     * @param value the value to increment
     */
    void increment(value_type value);

    /**
     * Set the most frequently clicked group label.
     * @param value the value to set, zero to clear the top label previously set.
     */
    void setTopLabel(value_type value);

    /**
     * Get the most frequently clicked group labels.
     * @param limit the max number of labels to get
     * @param valueVec store the values of each group label
     * @param freqVec the click count for each group label
     * @post @p freqVec is sorted in descending order.
     */
    void getFreqLabel(
        int limit,
        std::vector<value_type>& valueVec,
        std::vector<int>& freqVec
    ) const;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & valueFreqMap_;
        ar & topValue_;
        ar & topFreq_;
    }

private:
    /**
     * Whether @c topValue_ is set by @c setTopLabel() previously.
     * @return true for set, false for not set.
     */
    bool isSetTopLabel_() const {
        return topFreq_ < 0;
    }

private:
    /** property value id => click frequency */
    typedef std::map<value_type, int> ValueFreqMap;
    ValueFreqMap valueFreqMap_;

    /** the value with highest frequency */
    value_type topValue_;

    /** the highest frequency.
     * a non-negative @c topFreq_ means the frequency of @c topValue_ is highest,
     * a negative @c topFreq_ means @c topValue_ is set by @c setTopLabel().
     */
    int topFreq_;
};

}

#endif 

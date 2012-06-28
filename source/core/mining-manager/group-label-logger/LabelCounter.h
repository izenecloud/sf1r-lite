///
/// @file LabelCounter.h
/// @brief count the frequency for each property value
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2011-07-25
///

#ifndef SF1R_LABEL_COUNTER_H
#define SF1R_LABEL_COUNTER_H

#include <mining-manager/group-manager/PropValueTable.h> // pvid_t
#include <common/ClickCounter.h>
#include <vector>
#include <boost/serialization/access.hpp>

namespace sf1r
{

struct LabelCounter
{
    typedef faceted::PropValueTable::pvid_t LabelId;

    /** label id => click frequency */
    typedef ClickCounter<LabelId, int> FreqCounter;
    FreqCounter freqCounter_;

    std::vector<LabelId> setLabelIds_;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & freqCounter_;
        ar & setLabelIds_;
    }
};

} // namespace sf1r

#endif // SF1R_LABEL_COUNTER_H

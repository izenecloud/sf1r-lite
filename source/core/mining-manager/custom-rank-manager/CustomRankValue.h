/**
 * @file CustomRankValue.h
 * @brief the custom value for docs ranking.
 * @author Jun Jiang <jun.jiang@izenesoft.com>
 * @date Created 2012-07-31
 */

#ifndef SF1R_CUSTOM_RANK_VALUE_H
#define SF1R_CUSTOM_RANK_VALUE_H

#include <common/inttypes.h>
#include <vector>
#include <boost/serialization/access.hpp>

namespace sf1r
{

struct CustomRankValue
{
    typedef std::vector<docid_t> DocIdList;

    DocIdList topIds;

    DocIdList excludeIds;

    void clear();

    bool empty() const;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & topIds;
        ar & excludeIds;
    }
};

} // namespace sf1r

#endif // SF1R_CUSTOM_RANK_VALUE_H

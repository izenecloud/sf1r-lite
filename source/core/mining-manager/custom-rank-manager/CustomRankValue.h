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
#include <string>
#include <boost/serialization/access.hpp>

namespace sf1r
{

template <typename DocIdT>
struct CustomRankValue
{
    typedef std::vector<DocIdT> DocIdList;

    DocIdList topIds;

    DocIdList excludeIds;

    void clear()
    {
        topIds.clear();
        excludeIds.clear();
    }

    bool empty() const
    {
        return topIds.empty() && excludeIds.empty();
    }

    template <class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & topIds;
        ar & excludeIds;
    }
};

/// CustomRankValue doc id version
typedef CustomRankValue<docid_t> CustomRankDocId;

/// CustomRankValue doc string version
typedef CustomRankValue<std::string> CustomRankDocStr;

} // namespace sf1r

#endif // SF1R_CUSTOM_RANK_VALUE_H

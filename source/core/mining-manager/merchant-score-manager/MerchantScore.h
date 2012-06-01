///
/// @file MerchantScore.h
/// @brief the classes for merchant score, and category score
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2012-05-11
///

#ifndef SF1R_MERCHANT_SCORE_H
#define SF1R_MERCHANT_SCORE_H

#include <common/inttypes.h>
#include <string>
#include <map>

namespace sf1r
{

template <typename CategoryT>
struct CategoryScore
{
    typedef typename std::map<CategoryT, score_t> CategoryScoreMap;
    CategoryScoreMap categoryScoreMap;

    /**
     * if no category is found in @c categoryScoreMap,
     * the @c generalScore would be used.
     */
    score_t generalScore;

    CategoryScore() : generalScore(0) {}
};

template <typename MerchantT, typename CategoryT>
struct MerchantScoreMap
{
    typedef typename std::map<MerchantT, CategoryScore<CategoryT> > map_t;
    map_t map;

    void clear() { map.clear(); }
};

/// CategoryScore id version
typedef CategoryScore<category_id_t> CategoryIdScore;

/// CategoryScore string version
typedef CategoryScore<std::string> CategoryStrScore;

/// MerchantScoreMap id version
typedef MerchantScoreMap<merchant_id_t, category_id_t> MerchantIdScoreMap;

/// MerchantScoreMap string version
typedef MerchantScoreMap<std::string, std::string> MerchantStrScoreMap;

} // namespace sf1r

#endif // SF1R_MERCHANT_SCORE_H

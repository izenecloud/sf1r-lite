#ifndef SF1R_PRODUCT_SCORE_CONFIG_H_
#define SF1R_PRODUCT_SCORE_CONFIG_H_

#include <common/inttypes.h>
#include <string>
#include <vector>
#include <boost/serialization/access.hpp>

namespace sf1r
{

enum ProductScoreType
{
    MERCHANT_SCORE = 0,
    CUSTOM_SCORE,
    CATEGORY_SCORE,
    RELEVANCE_SCORE,
    POPULARITY_SCORE,
    FUZZY_SCORE,
    OFFER_ITEM_COUNT_SCORE,
    DIVERSITY_SCORE,
    RANDOM_SCORE,
    ZAMBEZI_RELEVANCE_SCORE,
    PRODUCT_SCORE_NUM
};

/**
 * @brief The configuration for <ProductRanking> <Score>.
 */
struct ProductScoreConfig
{
    ProductScoreType type;

    std::string propName;

    score_t weight;

    score_t minLimit;
    score_t maxLimit;
    score_t zoomin;

    score_t logBase;
    score_t mean;
    score_t deviation;

    bool isDebug; /// whether print debug message

    std::vector<ProductScoreConfig> factors;

    ProductScoreConfig();

    /**
     * Limit the @p score within the range [minLimit, maxLimit].
     */
    void limitScore(score_t& score) const;

    /**
     * @return true if @p score is within the range [minLimit, maxLimit].
     */
    bool isValidScore(score_t score) const;

    /**
     * @return a string which could be printed in debug.
     */
    std::string toStr() const;

private:
    friend class boost::serialization::access;

    template <typename Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & type;
        ar & propName;
        ar & weight;
        ar & minLimit;
        ar & maxLimit;
        ar & zoomin;
        ar & logBase;
        ar & mean;
        ar & deviation;
        ar & isDebug;
        ar & factors;
    }
};

} // namespace sf1r

#endif // SF1R_PRODUCT_SCORE_CONFIG_H_

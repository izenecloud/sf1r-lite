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
    CATEGORY_CLASSIFY_SCORE,
    RELEVANCE_SCORE,
    POPULARITY_SCORE,
    FUZZY_SCORE,
    OFFER_ITEM_COUNT_SCORE,
    DIVERSITY_SCORE,
    RANDOM_SCORE,
    TITLE_RELEVANCE_SCORE,
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

    bool isDebug; /// whether print debug message

    std::vector<ProductScoreConfig> factors;

    ProductScoreConfig();

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
        ar & isDebug;
        ar & factors;
    }
};

} // namespace sf1r

#endif // SF1R_PRODUCT_SCORE_CONFIG_H_

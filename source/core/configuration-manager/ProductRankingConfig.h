#ifndef SF1R_PRODUCT_RANKING_CONFIG_H_
#define SF1R_PRODUCT_RANKING_CONFIG_H_

#include "ProductScoreConfig.h"
#include <vector>
#include <map>
#include <string>
#include <boost/serialization/access.hpp>

namespace sf1r
{
class CollectionMeta;

/**
 * @brief The configuration for <ProductRanking>.
 */
class ProductRankingConfig
{
public:
    bool isEnable; /// whether enable product ranking

    std::vector<ProductScoreConfig> scores;

    ProductRankingConfig();

    /**
     * @param typeName the type name, such as "custom"
     * @return the type id, if no type id is found,
     *         PRODUCT_SCORE_NUM is returned.
     */
    ProductScoreType getScoreType(const std::string& typeName) const;

    /**
     * @return true for success, false for failure.
     */
    bool checkConfig(
        const CollectionMeta& collectionMeta,
        std::string& error) const;

    /**
     * @return a string which could be printed in debug.
     */
    std::string toStr() const;

private:
    /** key: score type name */
    typedef std::map<std::string, ProductScoreType> ScoreTypeMap;
    ScoreTypeMap scoreTypeMap;

    friend class boost::serialization::access;

    template <typename Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & isEnable;
        ar & scores;
        ar & scoreTypeMap;
    }
};

} // namespace sf1r

#endif // SF1R_PRODUCT_RANKING_CONFIG_H_

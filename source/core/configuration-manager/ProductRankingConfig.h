#ifndef SF1R_PRODUCT_RANKING_CONFIG_H_
#define SF1R_PRODUCT_RANKING_CONFIG_H_

#include <string>
#include <vector>
#include <boost/serialization/access.hpp>

namespace sf1r
{

/**
 * @brief The configuration for product ranking.
 */
class ProductRankingConfig
{
public:
    /// whether enable product ranking
    bool isEnable;

    /// whether print debug info
    bool isDebug;

    /// merchant property name, used for merchant score and diversity
    std::string merchantPropName;

    /// category property name, used for category score
    std::string categoryPropName;

    /// to calculate category score, also use other property names
    /// such as "Price", etc
    std::vector<std::string> categoryScorePropNames;

    ProductRankingConfig() : isEnable(false), isDebug(false) {}

private:
    friend class boost::serialization::access;

    template <typename Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & isEnable;
        ar & isDebug;
        ar & merchantPropName;
        ar & categoryPropName;
        ar & categoryScorePropNames;
    }
};

} // namespace sf1r

#endif // SF1R_PRODUCT_RANKING_CONFIG_H_

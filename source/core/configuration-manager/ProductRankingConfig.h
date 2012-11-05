#ifndef SF1R_PRODUCT_RANKING_CONFIG_H_
#define SF1R_PRODUCT_RANKING_CONFIG_H_

#include <string>
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

    /// merchant property name, used for merchant score
    std::string merchantPropName;

    /// category property name, used for category score
    std::string categoryPropName;

    ProductRankingConfig() : isEnable(false) {}

private:
    friend class boost::serialization::access;

    template <typename Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & isEnable;
        ar & merchantPropName;
        ar & categoryPropName;
    }
};

} // namespace sf1r

#endif // SF1R_PRODUCT_RANKING_CONFIG_H_

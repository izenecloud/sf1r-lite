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
    /// merchant property name, used for merchant score and diversity
    std::string merchantPropName;

    /// category property name, used for category boosting
    std::string categoryPropName;

    /// sub property name, used for category boosting
    std::string boostingSubPropName;

    /// whether print debug info
    bool isDebug;

    ProductRankingConfig() : isDebug(false) {}

    bool isEnable() const
    {
        return !merchantPropName.empty() ||
               !categoryPropName.empty();
    }

private:
    friend class boost::serialization::access;

    template <typename Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & merchantPropName;
        ar & categoryPropName;
        ar & boostingSubPropName;
    }
};

} // namespace sf1r

#endif // SF1R_PRODUCT_RANKING_CONFIG_H_

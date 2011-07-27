#ifndef SF1R_PROPERTY_RERANK_CONFIG_H_
#define SF1R_PROPERTY_RERANK_CONFIG_H_

#include <string>
#include <boost/serialization/access.hpp>

namespace sf1r
{

/**
 * @brief The type of attr property configuration.
 */
class PropertyRerankConfig
{
public:
    /// property name
    std::string propName;
    /// boosting property name
    std::string boostingPropName;
private:
    friend class boost::serialization::access;

    template <typename Archive>
    void serialize( Archive & ar, const unsigned int version )
    {
        ar & propName;
        ar & boostingPropName;
    }
};

} // namespace

#endif //SF1R_PROPERTY_RERANK_CONFIG_H_


#ifndef SF1R_SUMMARIZATION_CONFIG_H_
#define SF1R_SUMMARIZATION_CONFIG_H_

#include <string>

#include <boost/serialization/access.hpp>

namespace sf1r
{

/**
 * @brief The type of attr property configuration.
 */
class SummarizeConfig
{
public:
    /// log path that store parent key logs (SCD format)
    std::string parentKeyLogPath;
    std::string parentKey;
    /// property name
    std::string foreignKeyPropName;
    std::string contentPropName;

    friend class boost::serialization::access;

    template <typename Archive>
    void serialize( Archive & ar, const unsigned int version )
    {
        ar & parentKeyLogPath;
        ar & parentKey;
        ar & foreignKeyPropName;
        ar & contentPropName;
    }
};

} // namespace

#endif //_ATTR_CONFIG_H_

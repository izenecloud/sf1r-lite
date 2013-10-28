#ifndef SF1R_AD_INDEX_CONFIG_H_
#define SF1R_AD_INDEX_CONFIG_H_

#include <string>
#include <boost/serialization/access.hpp>

namespace sf1r
{

/**
 * @brief The configuration for <Ad Index>.
 */
class AdIndexConfig
{
public:
    bool isEnable;

    std::string indexFilePath;

    std::string clickPredictorWorkingPath;

    AdIndexConfig() : isEnable(false)
    {}

private:
    friend class boost::serialization::access;

    template <typename Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & isEnable;
        ar & indexFilePath;
    }
};

} // namespace sf1r

#endif // SF1R_AD_INDEX_CONFIG_H_

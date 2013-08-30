#ifndef SF1R_ZAMBEZI_CONFIG_H_
#define SF1R_ZAMBEZI_CONFIG_H_

#include <string>
#include <boost/serialization/access.hpp>

namespace sf1r
{

/**
 * @brief The configuration for <Zambezi>.
 */
class ZambeziConfig
{
public:
    bool isEnable;

    std::string indexPropName;

    ZambeziConfig() : isEnable(false) {}

private:
    friend class boost::serialization::access;

    template <typename Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & isEnable;
        ar & indexPropName;
    }
};

} // namespace sf1r

#endif // SF1R_ZAMBEZI_CONFIG_H_

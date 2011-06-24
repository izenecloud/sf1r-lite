#ifndef SF1R_ATTR_CONFIG_H_
#define SF1R_ATTR_CONFIG_H_

#include <string>
#include <boost/serialization/access.hpp>

namespace sf1r
{

/**
 * @brief The type of attr property configuration.
 */
class AttrConfig
{
public:
    /// property name
    std::string propName;

private:
    friend class boost::serialization::access;

    template <typename Archive>
    void serialize( Archive & ar, const unsigned int version )
    {
        ar & propName;
    }
};

} // namespace

#endif //_ATTR_CONFIG_H_

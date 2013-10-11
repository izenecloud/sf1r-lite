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
    bool reverse;

    uint32_t poolSize;
    uint32_t poolCount;

    std::string indexFilePath;

    ZambeziConfig() : isEnable(false)
                    , reverse(false)
                    , poolSize(0)
                    , poolCount(0)
    {}

private:
    friend class boost::serialization::access;

    template <typename Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & isEnable;
        ar & reverse;
        ar & poolSize;
        ar & poolCount;
        ar & indexFilePath;
    }
};

} // namespace sf1r

#endif // SF1R_ZAMBEZI_CONFIG_H_

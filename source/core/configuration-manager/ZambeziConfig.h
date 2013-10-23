#ifndef SF1R_ZAMBEZI_CONFIG_H_
#define SF1R_ZAMBEZI_CONFIG_H_

#include <string>
#include <boost/serialization/access.hpp>

namespace sf1r
{

/**
 * @brief The configuration for <Zambezi>.
 */

struct zambeziProperty
{
    std::string name;
    std::string tokenPath;
    uint32_t poolSize;
    float weight;

    zambeziProperty()
    : poolSize(268435456) //256M
    , weight(1)
    {
    }

    void display()
    {
        std::cout << "name:" << name << " ,tokenPath:" << tokenPath
            << " ,poolSize:" << poolSize << " ,weight:" << weight << std::endl;   
    }

};

struct zambeziVirtualProperty
{
    std::string name;
    std::vector<std::string> subProperties;
    std::string tokenPath;
    uint32_t poolSize;
    float weight;
    bool isAttrToken;

    zambeziVirtualProperty()
    : poolSize(268435456) //256M
    , weight(1)
    , isAttrToken(false)
    {
    }

    void display()
    {
        std::cout << "name:" << name << " ,tokenPath:" << tokenPath
            << " ,poolSize:" << poolSize << " ,weight:" << weight
            << " ,isAttrToken" << isAttrToken << std::endl;
        std::cout << "subProperties:";
        for (unsigned int i = 0; i < subProperties.size(); ++i)
        {
            std::cout << subProperties[i] << '\t';   
        }
        std::cout << std::endl;
    }
};

class ZambeziConfig
{
public:
    bool isEnable;
    bool reverse;

    uint32_t poolSize;
    uint32_t poolCount;

    std::string indexFilePath;

    std::vector<zambeziProperty> properties;

    std::vector<zambeziVirtualProperty> virtualPropeties;

    ZambeziConfig() : isEnable(false)
                    , reverse(false)
                    , poolCount(0)
    {}

    void display()
    {
        std::cout << "zambezi search cnfig:" << std::endl;
        std::cout << "isEnable:" << isEnable << std::endl;
        std::cout << "reverse:" << reverse << std::endl;
        std::cout << "poolSize: "<< poolSize << std::endl;
        std::cout << "poolCount: "<< poolCount << std::endl;
        std::cout << "indexFilePath: "<< indexFilePath << std::endl;

        for (std::vector<zambeziProperty>::iterator i = properties.begin(); i != properties.end(); ++i)
            i->display();

        for (std::vector<zambeziVirtualProperty>::iterator i = virtualPropeties.begin(); i != virtualPropeties.end(); ++i)
            i->display();
    }

private:
    friend class boost::serialization::access;

    template <typename Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & isEnable;
        ar & reverse;
        ar & poolCount;
    }
};

} // namespace sf1r

#endif // SF1R_ZAMBEZI_CONFIG_H_

#ifndef SF1R_ZAMBEZI_CONFIG_H_
#define SF1R_ZAMBEZI_CONFIG_H_

#include <string>
#include <boost/serialization/access.hpp>
#include <map>
#include <list>
#include <iostream>
#include <vector>

namespace sf1r
{

/**
 * @brief The configuration for <Zambezi>.
 */

struct ZambeziProperty
{
    std::string name;
    uint32_t poolSize;
    float weight;

    ZambeziProperty()
    : poolSize(268435456) //256M
    , weight(1)
    {
    }

    void display()
    {
        std::cout << "name:" << name 
            << " ,poolSize:" << poolSize << " ,weight:" << weight << std::endl;   
    }

};

struct ZambeziVirtualProperty
{
    std::string name;
    std::vector<std::string> subProperties;
    uint32_t poolSize;
    float weight;
    bool isAttrToken;

    ZambeziVirtualProperty()
    : poolSize(268435456) //256M
    , weight(1)
    , isAttrToken(false)
    {
    }

    void display()
    {
        std::cout << "name:" << name
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

struct PropertyStatus
{
    bool isCombined;
    bool isAttr;
    PropertyStatus()
    : isCombined(false)
    , isAttr(false)
    {
    }

    void display()
    {
        std::cout << "  isCombined:" << isCombined << "  ,isAttr:" << isAttr << std::endl; 
    }
};


class ZambeziConfig
{
public:
    bool isEnable;
    bool reverse;

    uint32_t poolSize;
    uint32_t poolCount;

    std::string tokenPath;
    std::string indexFilePath;

    std::string system_resource_path_;

    std::vector<ZambeziProperty> properties;

    std::vector<ZambeziVirtualProperty> virtualPropeties;

    std::map<std::string, PropertyStatus> property_status_map;

    bool hasAttrtoken;

    ZambeziConfig() : isEnable(false)
                    , reverse(false)
                    , poolCount(0)
                    , hasAttrtoken(false)
    {}


    void display()
    {
        std::cout << "ZAMBEZI SEARCH CONFIG..." << std::endl;
        std::cout << "isEnable:" << isEnable << std::endl;
        std::cout << "reverse:" << reverse << std::endl;
        std::cout << "poolSize: "<< poolSize << std::endl;
        std::cout << "poolCount: "<< poolCount << std::endl;
        std::cout << "indexFilePath: "<< indexFilePath << std::endl;
        std::cout << "tokenPath: "<< tokenPath << std::endl;
        std::cout << "hasAttrtoken: "<< hasAttrtoken << std::endl;
        std::cout << "system_resource_path_: "<< system_resource_path_ << std::endl;

        for (std::vector<ZambeziProperty>::iterator i = properties.begin(); i != properties.end(); ++i)
            i->display();

        for (std::vector<ZambeziVirtualProperty>::iterator i = virtualPropeties.begin(); i != virtualPropeties.end(); ++i)
            i->display();

        for (std::map<std::string, PropertyStatus>::iterator i = property_status_map.begin(); i != property_status_map.end(); ++i)
        {
            std::cout << "Property:" << i->first; 
            i->second.display();
        }
    }

    bool checkConfig()
    {
        if (hasAttrtoken)
            return ((virtualPropeties.size() == 1) && (properties.empty()));
        return true;
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

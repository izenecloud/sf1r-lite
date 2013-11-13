#ifndef SF1R_ZAMBEZI_CONFIG_H_
#define SF1R_ZAMBEZI_CONFIG_H_

#include <string>
#include <boost/serialization/access.hpp>
#include <map>
#include <list>
#include <iostream>
#include <vector>
#include "PropertyConfig.h"

namespace sf1r
{
/**
 * @brief : this is the base configuration for Zambezi index;
 *
 */

struct ZambeziProperty
{
    std::string name;
    uint32_t poolSize;
    float weight;
    bool isFilter;
    sf1r::PropertyDataType type;

    ZambeziProperty()
    : poolSize(268435456) //256M
    , weight(1.0)
    , isFilter(false)
    {
    }

    void display()
    {
        std::cout << "name:" << name 
            << " ,poolSize:" << poolSize << " ,weight:" << weight << " ,isFilter:" << isFilter <<std::endl;   
    }
};

/**
 * @brief : this is the configuration for Zambezi index virtual preporty;
 *
 */
class ZambeziVirtualProperty : public ZambeziProperty
{
public:
    /*
    * @brief: all the property used to build virtual property must be put in subProperties;
    */
    std::vector<std::string> subProperties;
    sf1r::PropertyDataType type;
    bool isAttrToken;

    ZambeziVirtualProperty()
    {
    }

    void display()
    {
        std::cout << "name:" << name
            << " ,poolSize:" << poolSize << " ,weight:" << weight
            << " ,isFilter:" << isFilter
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
    bool isFilter;
    PropertyStatus()
    : isCombined(false)
    , isAttr(false)
    , isFilter(false)
    {
    }

    void display()
    {
        std::cout << "  isCombined:" << isCombined << "  ,isAttr:" << isAttr << std::endl; 
    }
};

/*
 * @brief the integrated configuration for zambezi index;
 *
 */
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

    std::set<PropertyConfig, PropertyComp> ZambeziIndexSchema;

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
        if (!setZambeziIndexSchema()) // make sure there is no duplicate property;
            return false;

        if (hasAttrtoken)
            return ((virtualPropeties.size() == 1) && (properties.empty()));
        return true;
    }

    bool checkValidationConfig(const std::string& property) const
    {
        for (std::vector<ZambeziProperty>::const_iterator i = properties.begin(); i != properties.end(); ++i)
            if (property == i->name)
                return true;

        for (std::vector<ZambeziVirtualProperty>::const_iterator i = virtualPropeties.begin(); i != virtualPropeties.end(); ++i)
            if (property == i->name)
                return true;

        return false;
    }

    bool setZambeziIndexSchema()
    {
        for (std::vector<ZambeziProperty>::iterator i = properties.begin(); i != properties.end(); ++i)
        {
            PropertyConfig tmpConfig;

            tmpConfig.setRankWeight(i->weight);
            tmpConfig.setIsFilter(i->isFilter);
            tmpConfig.setName(i->name);
            tmpConfig.setType(i->type);
            //TODO add isIndexed;

            if (!ZambeziIndexSchema.insert(tmpConfig).second)
                return false;
        }

        for (std::vector<ZambeziVirtualProperty>::iterator i = virtualPropeties.begin(); i != virtualPropeties.end(); ++i)
        {
            PropertyConfig tmpConfig;
            tmpConfig.setRankWeight(i->weight);
            tmpConfig.setIsFilter(i->isFilter);
            tmpConfig.setName(i->name);
            tmpConfig.setType(i->type);
            //TODO add isIndexed;
            if (!ZambeziIndexSchema.insert(tmpConfig).second)
                return false;
        }

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

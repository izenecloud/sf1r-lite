/**
 * @file ZambeziConfig.h
 * @brief Configuration for zambezi index;
 * @author Hongliang Zhao
 * @date 2013-11.12
 */

#ifndef SF1R_ZAMBEZI_CONFIG_H_
#define SF1R_ZAMBEZI_CONFIG_H_

#include <string>
#include <glog/logging.h>
#include <boost/serialization/access.hpp>
#include <map>
#include <list>
#include <iostream>
#include <vector>
#include "PropertyConfig.h"

namespace sf1r
{

struct ZambeziIndexType
{
    enum IndexType
    {
        DefultIndexType = 0,
        PostionIndexType
    };
};

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
    bool isTokenizer;
    sf1r::PropertyDataType type;

    ZambeziProperty()
    : poolSize(268435456) //256M
    , weight(1.0)
    , isFilter(false)
    , isTokenizer(false)
    {
    }

    void display()
    {
        std::cout << "name:" << name 
            << " ,poolSize:" << poolSize 
            << " ,weight:" << weight 
            << " ,isFilter:" << isFilter 
            << ",isTokenizer:" << isTokenizer 
            <<std::endl;   
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
    * @brief: all the property used to build virtual property 
    *         must be put in subProperties;
    */
    std::vector<std::string> subProperties;
    sf1r::PropertyDataType type;
    bool isAttrToken;

    ZambeziVirtualProperty()
    : type(sf1r::UNKNOWN_DATA_PROPERTY_TYPE)
    , isAttrToken(false)
    {
    }

    void display()
    {
        std::cout << "name:" << name
            << " ,poolSize:" << poolSize 
            << " ,weight:" << weight
            << " ,isFilter:" << isFilter
            << " ,isAttrToken" << isAttrToken
            << std::endl;
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
    bool isTokenizer;
    float weight;
    PropertyStatus()
    : isCombined(false)
    , isAttr(false)
    , isFilter(false)
    , isTokenizer(false)
    , weight(0)
    {
    }

    void display()
    {
        std::cout << " :isCombined:" << isCombined 
        << " ,isAttr:" << isAttr
        << " ,isFilter:" << isFilter
        << " ,isTokenizer:" << isTokenizer
        << " ,weight:" << weight
        << std::endl;
    }
};

/*
 * @brief the integrated configuration for zambezi index;
 *
 */

typedef std::set<PropertyConfig, PropertyComp> ZambeziIndexSchema;
typedef std::map<std::string, PropertyStatus> ZambeziStatusMap;

class ZambeziConfig
{
public:
    bool isEnable;
    bool reverse;

    uint32_t poolSize;
    uint32_t poolCount;
    uint32_t vocabSize;

    std::string tokenPath;
    std::string indexFilePath;

    std::string system_resource_path_;

    ZambeziIndexType::IndexType indexType_;

    bool hasAttrtoken;

    /**
     * @brief: @properties, and @virtualPropeties
     * this is used to build zambezi index, and to build zambeziIndexSchema;
     * the two vectors are from each collection's xml config;
     */
    std::vector<ZambeziProperty> properties;
    std::vector<ZambeziVirtualProperty> virtualPropeties;

    ZambeziStatusMap property_status_map;

    /**
     * @brief: it it used in document-manager and docuemntSearch handler;
     */
    ZambeziIndexSchema zambeziIndexSchema;


    ZambeziConfig() : isEnable(false)
                    , reverse(false)
                    , poolSize(0)
                    , poolCount(0)
                    , vocabSize(1U << 25) // 32M terms in default
                    , indexType_(ZambeziIndexType::DefultIndexType)
                    , hasAttrtoken(false)
    {}


    void display()
    {
        std::cout << "ZAMBEZI SEARCH CONFIG..." << std::endl;
        std::cout << "isEnable:" << isEnable << std::endl;
        std::cout << "reverse:" << reverse << std::endl;
        std::cout << "poolSize: "<< poolSize << std::endl;
        std::cout << "poolCount: "<< poolCount << std::endl;
        std::cout << "vocabSize: "<< vocabSize << std::endl;
        std::cout << "indexFilePath: "<< indexFilePath << std::endl;
        std::cout << "tokenPath: "<< tokenPath << std::endl;
        std::cout << "indexType_" << indexType_ << std::endl;
        std::cout << "hasAttrtoken: "<< hasAttrtoken << std::endl;
        std::cout << "system_resource_path_: "<< system_resource_path_ << std::endl;

        /*for (std::vector<ZambeziProperty>::iterator i = properties.begin();
             i != properties.end(); ++i)
            i->display();

        for (std::vector<ZambeziVirtualProperty>::iterator i = virtualPropeties.begin(); 
            i != virtualPropeties.end(); ++i)
            i->display();*/

        for (ZambeziStatusMap::iterator i = property_status_map.begin();
             i != property_status_map.end(); ++i)
        {
            std::cout << "Property:" << i->first; 
            i->second.display();
        }
    }

    /**
     * @breif set ZambeziIndexSchema, 
     * and make sure if used attr_token, properties must be empty;
     * 
     **/
    bool checkConfig()
    {
        if (!setZambeziIndexSchema())
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

    /**
     * @brief set ZambeziIndexSchema after zambezi config is parsered;
     *        and, check there is no vialid property in zambezi config;
     *
     */
    bool setZambeziIndexSchema()
    {
        for (std::vector<ZambeziProperty>::iterator i = properties.begin(); i != properties.end(); ++i)
        {
            PropertyConfig tmpConfig;
            tmpConfig.setName(i->name);

            ZambeziIndexSchema::iterator sp = zambeziIndexSchema.find(tmpConfig);
            if (sp == zambeziIndexSchema.end())
            {
                LOG(ERROR) << "[ERROR]: the property:<" << i->name << "> does not exsit in document Schema";
                return false;
            }
            PropertyConfig propertyConfig(*sp);
            zambeziIndexSchema.erase(*sp);

            propertyConfig.setRankWeight(i->weight);
            propertyConfig.setIsFilter(i->isFilter);
            propertyConfig.setName(i->name);
            propertyConfig.setType(i->type);
            propertyConfig.setIsIndex(true);

            zambeziIndexSchema.insert(propertyConfig);
        }

        for (std::vector<ZambeziVirtualProperty>::iterator i = virtualPropeties.begin(); i != virtualPropeties.end(); ++i)
        {
            /**
             * @to support only Virtual Search;
             * only when the subProperty has not exsit in properties, 
             * add this subProperty to zambezIndexSchema;
             **/
            for (std::vector<std::string>::iterator j = i->subProperties.begin(); j != i->subProperties.end(); ++j)
            {
                if (!checkValidationConfig(*j))
                {
                    PropertyConfig tmpConfig;
                    tmpConfig.setName(*j);
                    ZambeziIndexSchema::iterator sp = zambeziIndexSchema.find(tmpConfig);
                    if (sp == zambeziIndexSchema.end())
                    {
                        LOG(ERROR) << "[ERROR]: the property:" << i->name << "> does not exsit in document Schema";
                        return false;
                    }

                    PropertyConfig propertyConfig(*sp);
                    zambeziIndexSchema.erase(*sp);

                    propertyConfig.setRankWeight(i->weight);
                    propertyConfig.setIsFilter(i->isFilter);
                    propertyConfig.setName(i->name);
                    propertyConfig.setType(i->type);
                    propertyConfig.setIsIndex(true);

                    zambeziIndexSchema.insert(propertyConfig);
                }
            }
        }
        return true;
    }

    float getWeight(const std::string& property) const
    {
        ZambeziStatusMap::const_iterator iter = property_status_map.find(property);
        if (iter != property_status_map.end())
        {
            return iter->second.weight;
        }
        return 0;
    }

private:
    friend class boost::serialization::access;

    template <typename Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & isEnable;
        ar & reverse;
        ar & poolSize;
        ar & poolCount;
        ar & vocabSize;
    }
};

} // namespace sf1r

#endif // SF1R_ZAMBEZI_CONFIG_H_

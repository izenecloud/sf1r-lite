#ifndef SF1V5_CONFIGURATION_MANAGER_COLLECTION_META_H
#define SF1V5_CONFIGURATION_MANAGER_COLLECTION_META_H
/**
 * @file configuration-manager/CollectionMeta.h
 * @date Created <2009-10-22 16:55:37>
 * @date Updated <2011-01-25 14:54:01>
 */
#include <common/type_defs.h>

#include "CollectionPath.h"
#include "MiningConfig.h"
#include "MiningSchema.h"
#include "IndexManagerConfig.h"
#include "SMiaConfig.h"
#include "PropertyConfig.h"
#include "Acl.h"

#include <util/ustring/UString.h>

#include <boost/serialization/access.hpp>
#include <boost/serialization/set.hpp>

#include <string>
#include <set>

namespace sf1r {

class CollectionMeta
{
    typedef std::set<PropertyConfig, PropertyComp> schema_type;

public:
    typedef schema_type::const_iterator property_config_const_iterator;
    typedef property_config_const_iterator property_config_iterator;

    CollectionMeta();
    ~CollectionMeta(){}

    //------------------------  Accessor  ------------------------

    void setColId(collectionid_t id)
    {
        colId_ = id;
    }
    collectionid_t getColId() const
    {
        return colId_;
    }
    void setName(std::string name)
    {
        //name_.swap(name);
    	name_ = name;
    }
    const std::string& getName() const
    {
        return name_;
    }
    void setEncoding(const izenelib::util::UString::EncodingType& encoding)
    {
        encoding_ = encoding;
    }
    const izenelib::util::UString::EncodingType& getEncoding() const
    {
        return encoding_;
    }

    void setWildcardType(std::string wildcardType) {wildcardType_ = wildcardType; }

    const std::string& getWildcardType() const {return wildcardType_;}

    void setGeneralClassifierLanguage(std::string generalClassifierLanguage) {generalClassifierLanguage_ = generalClassifierLanguage; }

    const std::string& getGeneralClassifierLanguage() const {return generalClassifierLanguage_;}

    const bool isUnigramWildcard() { return wildcardType_ == "unigram"; }

    const bool isTrieWildcard() {return wildcardType_ == "trie"; }

    void setRankingMethod(const std::string& rankingModel)
    {
        rankingModel_ = rankingModel;
    }
    const std::string& getRankingMethod() const
    {
        return rankingModel_;
    }
    void setDateFormat(const std::string& format)
    {
        dateFormat_ = format;
    }
    const std::string& getDateFormat() const
    {
        return dateFormat_;
    }
    void setCollectionPath(const CollectionPath& collPath)
    {
        collPath_ = collPath;
    }
    const CollectionPath& getCollectionPath() const
    {
        return collPath_;
    }

    void clearPropertyConfig()
    {
        schema_.clear();
    }

    /**
     * @brief adds one property config to schema
     * @return \c false if the name already existed, \c true if really inserted.
     */
    bool insertPropertyConfig(const PropertyConfig& property)
    {
        return schema_.insert(property).second;
    }

    /**
     * @brief erases by name
     * @return \c true if the property exists and has been erased.
     */
    bool erasePropertyConfig(const std::string& name)
    {
        PropertyConfig config;
        config.setName(name);

        return schema_.erase(config);
    }

    /**
     * @brief gets property by name
     * @return \c true if the property exists and the value has been assigned to
     *         \a config.
     */
    bool getPropertyConfig(const std::string& name,
                           PropertyConfig& config) const;
                           
    bool getPropertyType(const std::string& name,
                           PropertyDataType& type) const;
    /**
     * @brief gets property by name
     * @param[in,out] config config name should be set as input search key.
     * @return \c true if the property exists and the value has been assigned to
     *         \a config.
     */
    bool getPropertyConfig(PropertyConfig& config) const;

    const schema_type& getDocumentSchema() const
    {
        return schema_;
    }

    property_config_const_iterator propertyConfigBegin() const
    {
        return schema_.begin();
    }
    property_config_const_iterator propertyConfigEnd() const
    {
        return schema_.end();
    }

    /**
     * @brief returns true if the property is set with an Language Analyzer
     */
    bool getAnalysisInfo(
        const std::string& propertyName,
        AnalysisInfo& analysisInfo,
        std::string& analysis,
        std::string& language
    ) const;
    
    void setIndexManagerConfig(const IndexManagerConfig indexmanager_config)
    {
      indexmanager_config_ = indexmanager_config;
    }
   
    const IndexManagerConfig& getIndexManagerConfig() const
    {
      return indexmanager_config_;
    }
    
    void setSMiaConfig(const SMiaConfig& smia_config)
    {
      smia_config_ = smia_config;
    }
    const SMiaConfig& getSMiaConfig() const
    {
      return smia_config_;
    }
    
    void setMiningConfig(const MiningConfig& mining_config)
    {
      mining_config_ = mining_config;
    }
    const MiningConfig& getMiningConfig() const
    {
      return mining_config_;
    }
    
    void setMiningSchema(const MiningSchema& mining_schema)
    {
      mining_schema_ = mining_schema;
    }
    const MiningSchema& getMiningSchema() const
    {
      return mining_schema_;
    }

    /**
     * @brief finds the display setting of a property
     *
     * @param propertyName  The name of the property to search for
     * @param bSnipppet     The setting for snippet (true/false) is saved here
     * @param bHighlight    The setting for highlight (true/false) is saved here
     * @param bSummary      The setting for summary (true/false) is saved here
     *
     * @return  False if the setting found for the property is not found, or if
     *          the property does not exist
     */
    bool getAttributesInDisplayElement(
        const std::string& propertyName,
        bool& bSnippet,
        bool& bHighlight,
        bool& bSummary
    ) const;

    std::string toString() const;

    /**
     * @brief number property config in schema.
     *
     * Property id are numbered from 1 and by the name lexical order.
     */
    void numberPropertyConfig();

    void aclAllow(const string& tokens)
    {
        acl_.allow(tokens);
    }

    void aclDeny(const string& tokens)
    {
        acl_.deny(tokens);
    }

    const Acl& getAcl() const
    {
        return acl_;
    }

protected:
    friend class boost::serialization::access;
    template <class Archive>
    void serialize( Archive & ar, const unsigned int version )
    {
        ar & colId_;
        ar & name_;
        ar & encoding_;
        ar & rankingModel_;
        ar & wildcardType_;
        ar & generalClassifierLanguage_;
        ar & dateFormat_;
        ar & collPath_;
        ar & schema_;
        ar & smia_config_;
        ar & indexmanager_config_;
        ar & mining_config_;
        ar & mining_schema_;
        ar & acl_;
    }

private:
    /// @brief Collection ID
    collectionid_t colId_;

    /// @brief  The Collection's name given by the user
    std::string name_;

    /// @brief The encoding type of the Collection
    izenelib::util::UString::EncodingType encoding_;

    /// @brief  The id name of the ranking configuration unit used in this
    /// collection
    std::string rankingModel_;

    /// @brief how wildcard queries are processed, 'unigram' or 'trie'
    std::string wildcardType_;

    std::string generalClassifierLanguage_;

    std::string dateFormat_;

    /// @brief The base path of this collection. Currently used as SCD file
    /// location
    CollectionPath collPath_;

    /// @brief The DocumentSchemaConfig of this Collection
    std::set<PropertyConfig, PropertyComp> schema_;
    
    SMiaConfig smia_config_;
    IndexManagerConfig indexmanager_config_;
    MiningConfig mining_config_;
    MiningSchema mining_schema_;

    /// @brief Collection level acl_
    Acl acl_;
};

} // namespace sf1r

#endif // SF1V5_CONFIGURATION_MANAGER_COLLECTION_META_H

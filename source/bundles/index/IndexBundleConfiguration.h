#ifndef INDEX_BUNDLE_CONFIGURATION_H
#define INDEX_BUNDLE_CONFIGURATION_H

#include <configuration-manager/PropertyConfig.h>
#include <configuration-manager/RankingManagerConfig.h>
#include <configuration-manager/CollectionPath.h>

#include <ir/index_manager/utility/IndexManagerConfig.h>

#include <util/osgi/BundleConfiguration.h>
#include <util/ustring/UString.h>

namespace sf1r
{
typedef std::set<PropertyConfig, PropertyComp> IndexBundleSchema;
class IndexBundleConfiguration : public ::izenelib::osgi::BundleConfiguration
{
public:
    IndexBundleConfiguration(const std::string& collectionName);

    void setSchema(const std::set<PropertyConfigBase, PropertyBaseComp>& schema);

    void numberProperty();

    const bool isUnigramWildcard() { return wildcardType_ == "unigram"; }

    const bool isTrieWildcard() {return wildcardType_ == "trie"; }

    bool getPropertyConfig(const std::string& name, PropertyConfig& config) const;

    bool getAnalysisInfo(
        const std::string& propertyName,
        AnalysisInfo& analysisInfo,
        std::string& analysis,
        std::string& language
    ) const;

private:

    bool eraseProperty(const std::string& name)
    {
        PropertyConfig config;
        config.propertyName_ = name;
        return schema_.erase(config);
    }

public:
    std::string collectionName_;

    CollectionPath collPath_;
	
    /// Schema
    std::set<PropertyConfig, PropertyComp> schema_;

    std::string languageIdentifierDbPath_;
    /// Parameters
    /// @brief config for IndexManager
    izenelib::ir::indexmanager::IndexManagerConfig indexConfig_;

    /// @brief cron indexing expression
    std::string cronIndexer_;

    /// @brief whether trigger Question Answering mode
    bool bTriggerQA_;

    /// @brief document cache number
    size_t documentCacheNum_;

    /// @brief The encoding type of the Collection
    izenelib::util::UString::EncodingType encoding_;

    /// @brief how wildcard queries are processed, 'unigram' or 'trie'
    std::string wildcardType_;

    std::vector<std::string> collectionDataDirectories_;

    /// @brief	Configurations for RankingManager
    RankingManagerConfig rankingManagerConfig_;
};
}

#endif


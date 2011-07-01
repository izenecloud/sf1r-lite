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

enum IndexMultilangGranularity {
    FIELD_LEVEL, /// Each field adopts same language analzyer
    SENTENCE_LEVEL, /// Each sentence adopts same language analyzer
    BLOCK_LEVEL ///unsupported yet, which means reorganize text so that each block contains same language
}; 

class IndexBundleConfiguration : public ::izenelib::osgi::BundleConfiguration
{
public:
    IndexBundleConfiguration(const std::string& collectionName);

    void setSchema(const std::set<PropertyConfigBase, PropertyBaseComp>& schema);

    void numberProperty();

    const bool isUnigramWildcard() { return wildcardType_ == "unigram"; }

    const bool isTrieWildcard() {return wildcardType_ == "trie"; }

    const bool hasUnigramProperty() { return bIndexUnigramProperty_; }

    const bool isUnigramSearchMode() { return bUnigramSearchMode_; }

    void setIndexMultiLangGranularity(const std::string& granularity);

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

    /// @brief whether add unigram properties
    bool bIndexUnigramProperty_;

    /// @brief whether search based on unigram index terms
    bool bUnigramSearchMode_;

    /// @brief the granularity of multi language support during indexing
    IndexMultilangGranularity indexMultilangGranularity_;

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


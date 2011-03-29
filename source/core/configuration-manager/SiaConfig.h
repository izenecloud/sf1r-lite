#ifndef SMIA_CONFIG_H_
#define SMIA_CONFIG_H_

#include <string>
#include <boost/serialization/access.hpp>

namespace sf1r
{

class SiaConfig
{
public:
    SiaConfig()  {}
    ~SiaConfig() {}

    void setTriggerQA( bool flag )
    {
        bTriggerQA_ = flag;
    }

    bool isTriggerQA( ) const
    {
        return bTriggerQA_;
    }

    void setDocumentCacheNum(size_t& documentCacheNum)
    {
        documentCacheNum_ = documentCacheNum;
    }

    size_t getDocumentCacheNum() const
    {
        return documentCacheNum_;
    }


    std::vector<std::string>& collectionDataDirectories()
    {
        return collectionDataDirectories_;
    }

private:
    /// @brief whether trigger Question Answering mode
    bool bTriggerQA_;

    /// @brief document cache number
    size_t documentCacheNum_;

    /// @brief The encoding type of the Collection
    izenelib::util::UString::EncodingType encoding_;

    /// @brief  The id name of the ranking configuration unit used in this
    /// collection
    std::string rankingModel_;

    /// @brief how wildcard queries are processed, 'unigram' or 'trie'
    std::string wildcardType_;

    std::vector<std::string> collectionDataDirectories_;
}; // end - SiaConfig

} // end - namespace sf1r

#endif


#ifndef SMIA_CONFIG_H_
#define SMIA_CONFIG_H_

#include <string>
#include <boost/serialization/access.hpp>

namespace sf1r {

class SMiaConfig
{
public:
    SMiaConfig()  {}
    ~SMiaConfig() {}

    void setTriggerQA( bool flag )
    {
        bTriggerQA_ = flag;
    }

    bool isTriggerQA( ) const
    {
        return bTriggerQA_;
    }

    void setThreadNums(size_t& threadNum)
    {
        threadNum_ = threadNum;
    }

    size_t getThreadNums() const
    {
        return threadNum_;
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
    friend class boost::serialization::access;

    template <typename Archive>
    void serialize( Archive & ar, const unsigned int version ) 
    {
        ar & bTriggerQA_;
        ar & threadNum_;
        ar & documentCacheNum_;
        ar & collectionDataDirectories_;
    }

    bool bTriggerQA_;
    size_t threadNum_;
    size_t documentCacheNum_;
    std::vector<std::string> collectionDataDirectories_;
}; // end - SiaConfig

} // end - namespace sf1r

#endif


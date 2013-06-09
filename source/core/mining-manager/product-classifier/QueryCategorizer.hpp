#ifndef SF1R_MININGMANAGER_QUERY_CATEGORIZER_HPP_
#define SF1R_MININGMANAGER_QUERY_CATEGORIZER_HPP_

#include <string>
#include <vector>
#include <util/ustring/UString.h>
#include <cache/IzeneCache.h>

namespace sf1r
{
using izenelib::util::UString;

class ProductMatcher;
class SPUProductClassifier;
class SuffixMatchManager;
class DocumentManager;
class QueryCategorizer
{
enum ClassifierType
{
    MATCHER,
    SEARCH_SPU,
    SEARCH_PRODUCT
};

public:
    QueryCategorizer();
    ~QueryCategorizer();

    void SetProductMatcher(ProductMatcher* matcher)
    {
        matcher_ = matcher;
    }
/*    
    void SetSPUProductClassifier(SPUProductClassifier* spu_classifier)
    {
        spu_classifier_ = spu_classifier;
    }

    void SetSuffixMatchManager(SuffixMatchManager* suffix_manager)
    {
        suffix_manager_ = suffix_manager;
    }
*/
    void SetDocumentManager(boost::shared_ptr<DocumentManager> document_manager)
    {
        document_manager_ = document_manager;
    }

    void SetWorkingMode(std::string& mode);

    bool GetProductCategory(
        const std::string& query,
        int limit,
        std::vector<std::vector<std::string> >& pathVec);

private:
    bool GetCategoryByMatcher_(
        const std::string& query,
        int limit,
        std::vector<UString>& frontends);
/*
    bool GetCategoryBySPU_(
        const std::string& query,
        std::vector<UString>& frontCategories);

    bool GetCategoryBySuffixMatcher_(
        const std::string& query,
        std::vector<UString>& frontCategories);
*/
    bool GetSplittedCategories_(
        std::vector<UString>& frontends,
        int limit,
        std::vector<std::vector<std::string> >& pathVec);


    ProductMatcher* matcher_;
    SPUProductClassifier* spu_classifier_;
    SuffixMatchManager* suffix_manager_;
    boost::shared_ptr<DocumentManager> document_manager_;

    std::vector<ClassifierType> modes_;
    izenelib::cache::IzeneCache<std::string, std::vector<std::vector<std::string> >, izenelib::util::ReadWriteLock> cache_;
};

}


#endif


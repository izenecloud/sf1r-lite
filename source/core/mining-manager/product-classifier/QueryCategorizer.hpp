#ifndef SF1R_MININGMANAGER_QUERY_CATEGORIZER_HPP_
#define SF1R_MININGMANAGER_QUERY_CATEGORIZER_HPP_

#include <string>
#include <vector>
#include <util/ustring/UString.h>


namespace sf1r
{
using izenelib::util::UString;

class ProductMatcher;
class SPUProductClassifier;
class SuffixMatchManager;
class QueryCategorizer
{
public:
    QueryCategorizer();
    ~QueryCategorizer();

    void SetProductMatcher(ProductMatcher* matcher) { matcher_ = matcher; }
    void SetSPUProductClassifier(SPUProductClassifier* spu_classifier) { spu_classifier_ = spu_classifier;}
    void SetSuffixMatchManager(SuffixMatchManager* suffix_manager) { suffix_manager_ = suffix_manager;}

    bool GetProductCategory(
        const std::string& query,
        int limit,
        std::vector<std::vector<std::string> >& pathVec);

private:
    bool GetCategoryByMatcher_(
        const std::string& query,
        int limit,
        std::vector<UString>& frontends);

    bool GetCategoryBySPU_(
        const std::string& query,
        std::vector<UString>& frontCategories);

    bool GetCategoryBySuffixMatcher_(
        const std::string& query,
        std::vector<UString>& frontCategories);

    bool GetSplittedCategories_(
        std::vector<UString>& frontends,
        int limit,
        std::vector<std::vector<std::string> >& pathVec);


    ProductMatcher* matcher_;
    SPUProductClassifier* spu_classifier_;
    SuffixMatchManager* suffix_manager_;
};

}


#endif


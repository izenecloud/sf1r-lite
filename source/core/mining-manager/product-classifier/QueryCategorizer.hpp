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
class SuffixMatchManager;
class DocumentManager;
class QueryCategorizer
{

public:
    QueryCategorizer();
    ~QueryCategorizer();

    void SetProductMatcher(ProductMatcher* matcher)
    {
        matcher_ = matcher;
    }

    bool GetProductCategory(
        const std::string& query,
        int limit,
        std::vector<std::vector<std::string> >& pathVec);

private:
    bool GetCategoryByMatcher_(
        const std::string& query,
        int limit,
        std::vector<UString>& frontends);

    bool GetSplittedCategories_(
        std::vector<UString>& frontends,
        int limit,
        std::vector<std::vector<std::string> >& pathVec);


    ProductMatcher* matcher_;

    izenelib::cache::IzeneCache<std::string, std::vector<std::vector<std::string> >, izenelib::util::ReadWriteLock> cache_;
};

}


#endif


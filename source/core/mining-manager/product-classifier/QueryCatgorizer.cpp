#include "QueryCategorizer.hpp"
#include "SPUProductClassifier.hpp"
#include <mining-manager/suffix-match-manager/SuffixMatchManager.hpp>
#include <mining-manager/util/split_ustr.h>
#include <b5m-manager/product_matcher.h>

#include <glog/logging.h>
#include <set>

namespace sf1r
{

bool HasCategoryPrefix(
    const std::vector<UString>& category, 
    std::set<std::vector<UString> >& categories)
{
    std::set<std::vector<UString> >::iterator cit = categories.begin();
    for(; cit != categories.end(); ++cit)
    {
        const std::vector<UString>& c = *cit;
        size_t levels = std::min(category.size(), c.size());
        size_t i = 0;
        bool isPrefix = true;
        for(; i < levels; ++i)
        {
            if(category[i] != c[i])
            {
                isPrefix = false;
                break;
            }
        }
        if(isPrefix) return true;
    }
    return false;
}

QueryCategorizer::QueryCategorizer()
    :matcher_(NULL)
    ,spu_classifier_(NULL)
    ,suffix_manager_(NULL)
{
}

QueryCategorizer::~QueryCategorizer()
{
}

bool QueryCategorizer::GetCategoryByMatcher_(
    const std::string& query,
    int limit,
    std::vector<UString>& frontends)
{
    if(!matcher_) return false;
    Document doc;
    UString queryU(query, UString::UTF_8);
    doc.property("Title") = queryU;
	
    std::vector<ProductMatcher::Product> result_products;
    ProductMatcher* matcher = ProductMatcherInstance::get();
	
    if (matcher->Process(doc, (uint32_t)limit, result_products))
    {
        for(uint32_t i=0;i<result_products.size();i++)
        {
            const std::string& category_name = result_products[i].scategory;
            if (!category_name.empty())
            {
                if(!result_products[i].fcategory.empty())
                {
                    frontends.push_back(UString(result_products[i].fcategory, UString::UTF_8));
                }
            }
        }
    }
    return frontends.empty() ? false : true;
}

bool QueryCategorizer::GetCategoryBySPU_(
    const std::string& query,
    std::vector<UString>& frontCategories)
{
    if(!spu_classifier_) return false;

    return spu_classifier_->GetProductCategory(query, frontCategories);
}

bool QueryCategorizer::GetCategoryBySuffixMatcher_(
    const std::string& query,
    std::vector<UString>& frontCategories)
{
    if(!suffix_manager_) return false;

    return true;
}

bool QueryCategorizer::GetSplittedCategories_(
    std::vector<UString>& frontends,
    int limit,
    std::vector<std::vector<std::string> >& pathVec)
{
    if(frontends.empty()) return false;

    std::set<std::vector<UString> > splited_cat_set;
    for(std::vector<UString>::const_iterator it = frontends.begin();
        it != frontends.end(); ++it)
    {
        UString frontendCategory = *it;
        if(frontendCategory.empty()) continue;
		
        std::vector<std::vector<UString> > groupPaths;
        split_group_path(frontendCategory, groupPaths);
        if (groupPaths.empty()) continue;

        std::vector<std::string> path;
        const std::vector<UString>& topGroup = groupPaths[0];
        if(HasCategoryPrefix(topGroup, splited_cat_set)) continue;
        splited_cat_set.insert(topGroup);

        for (std::vector<UString>::const_iterator it = topGroup.begin();
             it != topGroup.end(); ++it)
        {
            std::string str;
            it->convertString(str, UString::UTF_8);
            path.push_back(str);
        }
        pathVec.push_back(path);
    }
    if(pathVec.size() > (unsigned)limit) pathVec.resize(limit);
    return true;
}

bool QueryCategorizer::GetProductCategory(
        const std::string& query,
        int limit,
        std::vector<std::vector<std::string> >& pathVec)
{
    std::string enriched_query;
    std::vector<UString> frontCategories;
    assert(spu_classifier_);
    spu_classifier_->GetEnrichedQuery(query, enriched_query);
    LOG(INFO)<<"GetEnrichedQuery "<<enriched_query;

    GetCategoryByMatcher_(enriched_query, limit, frontCategories);
    LOG(INFO)<<"GetCategoryByMatcher "<<frontCategories.size();

    GetCategoryBySPU_(enriched_query, frontCategories);
    LOG(INFO)<<"GetCategoryBySPU "<<frontCategories.size();

    GetCategoryBySuffixMatcher_(enriched_query, frontCategories);

    return GetSplittedCategories_(frontCategories, limit, pathVec);
}

}

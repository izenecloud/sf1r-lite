#include "QueryCategorizer.hpp"
#include "SPUProductClassifier.hpp"
#include <mining-manager/suffix-match-manager/SuffixMatchManager.hpp>
#include <mining-manager/util/split_ustr.h>
#include <document-manager/DocumentManager.h>
#include <b5m-manager/product_matcher.h>
#include <query-manager/ActionItem.h>

#include <util/ClockTimer.h>

#include <boost/algorithm/string.hpp>
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
    ,cache_(10000)
{
    //modes_.push_back(MATCHER);
    //modes_.push_back(SEARCH_PRODUCT);	
    //modes_.push_back(SEARCH_SPU);
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
    if(query.empty()) return false;

    Document doc;
    UString queryU(query, UString::UTF_8);
    doc.property("Title") = queryU;

    std::vector<ProductMatcher::Product> result_products;
    ProductMatcher* matcher = ProductMatcherInstance::get();
    matcher->GetFrontendCategory(queryU, (uint32_t)limit, frontends);

    return frontends.empty() ? false : true;
}

bool QueryCategorizer::GetSplittedCategories_(
    std::vector<UString>& frontends,
    int limit,
    std::vector<std::vector<std::string> >& pathVec)
{
    if(frontends.empty()) return false;
    pathVec.clear();
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

void QueryCategorizer::SetWorkingMode(std::string& mode)
{
    if(mode.empty()) return;
    LOG(INFO)<<"SetWorkingMode "<<mode;
	
    modes_.clear();
    std::vector<std::string> modes;
    boost::algorithm::split( modes, mode, boost::algorithm::is_any_of("+") );
    for(unsigned i = 0; i < modes.size(); ++i)	
    {
        if(modes[i] == "M") modes_.push_back(MATCHER);
        else if(modes[i] == "S") modes_.push_back(SEARCH_SPU);
        else if(modes[i] == "P") modes_.push_back(SEARCH_PRODUCT);
    }
}

bool QueryCategorizer::GetProductCategory(
    const std::string& query,
    int limit,
    std::vector<std::vector<std::string> >& pathVec)
{
    if (cache_.getValue(query, pathVec)) return !pathVec.empty();

    std::vector<UString> frontCategories;
    assert(spu_classifier_);

    GetCategoryByMatcher_(query, limit, frontCategories);
    if(frontCategories.size() >= (unsigned)limit)
    {
        for(unsigned i = 0; i < frontCategories.size(); ++i)    
        {
            std::string str;
            frontCategories[i].convertString(str, UString::UTF_8);
            LOG(INFO)<<"GetCategoryByMatcher "<<str;
        }
        GetSplittedCategories_(frontCategories, limit, pathVec);
        if(pathVec.size() == (unsigned)limit) 
        {
            cache_.insertValue(query, pathVec);
            return true;
        }
    }
    
/*    if(!modes_.empty()&&frontCategories.empty())
    {
        std::string enriched_query;
        spu_classifier_->GetEnrichedQuery(query, enriched_query);

        for(unsigned i = 0; i < modes_.size(); ++i)
        {
            switch(modes_[i])
            {
            case MATCHER:
                //GetCategoryByMatcher_(query, limit, frontCategories);
                //LOG(INFO)<<"GetCategoryByMatcher "<<frontCategories.size();
                break;
            case SEARCH_SPU:
                GetCategoryBySPU_(enriched_query, frontCategories);
                LOG(INFO)<<"GetCategoryBySPU "<<frontCategories.size();
                break;
            case SEARCH_PRODUCT:
                GetCategoryBySuffixMatcher_(enriched_query, frontCategories);
                LOG(INFO)<<"GetCategoryByProduct "<<frontCategories.size();
                break;
            }
        }
    }*/
    bool ret = GetSplittedCategories_(frontCategories, limit, pathVec);
    cache_.insertValue(query, pathVec);
    return ret;
}

}

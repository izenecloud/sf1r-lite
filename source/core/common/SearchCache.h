#ifndef CORE_COMMON_SEARCH_CACHE_H
#define CORE_COMMON_SEARCH_CACHE_H
/**
 * @file core/common/SearchCache.h
 * @author Ian Yang
 * @date Created <2009-09-30 09:31:29>
 * @date Updated <2010-03-24 15:43:04>
 */
#include "ResultType.h" // KeywordSearchResult
#include <query-manager/QueryIdentity.h>
#include <mining-manager/faceted-submanager/ontology_rep.h>
#include <mining-manager/faceted-submanager/ctr_manager.h>
#include <cache/IzeneCache.h>
#include <cache/concurrent_cache.hpp>

#include <vector>
#include <deque>

namespace sf1r
{

class SearchCache
{
public:
    typedef QueryIdentity key_type;
    typedef KeywordSearchResult value_type;
    //typedef izenelib::cache::IzeneCache<
    //    key_type,
    //    value_type,
    //    izenelib::util::ReadWriteLock,
    //    izenelib::cache::RDE_HASH,
    //    izenelib::cache::LRLFU
    //> cache_type;
    typedef izenelib::concurrent_cache::ConcurrentCache<key_type, value_type> cache_type;

    explicit SearchCache(unsigned cacheSize, time_t refreshInterval = 60*60, bool refreshAll = false)
        : cache_(cacheSize, izenelib::cache::LRLFU), special_cache_(cacheSize, izenelib::cache::LRLFU)
        , refreshInterval_(refreshInterval)
        , refreshAll_(refreshAll)
    {
    }

    /**
     * Cannot be \c const becuase \c IzeneCache::getValue is not const.
     */
    bool get(const key_type& key,
             value_type& result)
    {
        if (result.distSearchInfo_.nodeType_ == DistKeywordSearchInfo::NODE_WORKER)
            return false;

        value_type value;
        cache_type* pcache = &cache_;
        if (IsSpecialSearch(key))
        {
            pcache = &special_cache_;
        }
        if ((*pcache).get(key, value))
        {
            if (!needRefresh(key, value.timeStamp_))
            {
                result.swap(value);
                return true;
            }
        }

        return false;
    }

    void set(const key_type& key,
             value_type& result)
    {
        if (result.distSearchInfo_.nodeType_ == DistKeywordSearchInfo::NODE_WORKER)
            return;

        result.timeStamp_ = std::time(NULL);
        result.groupRep_.toOntologyRepItemList();

        // Temporarily store the summary results to keep them out of cache
        std::vector<std::vector<PropertyValue::PropertyValueStrType> > fullText, snippetText, rawText;

        fullText.swap(result.fullTextOfDocumentInPage_);
        snippetText.swap(result.snippetTextOfDocumentInPage_);
        rawText.swap(result.rawTextOfSummaryInPage_);
        cache_type* pcache = &cache_;
        if (IsSpecialSearch(key))
        {
            pcache = &special_cache_;
        }

        (*pcache).insert(key, result);

        fullText.swap(result.fullTextOfDocumentInPage_);
        snippetText.swap(result.snippetTextOfDocumentInPage_);
        rawText.swap(result.rawTextOfSummaryInPage_);
    }

    void clear()
    {
        cache_.clear();
        special_cache_.clear();
    }

private:
    /**
     * For keys with static values, we need not to refresh cache;
     * but for those with volatile values, we need to refresh cache periodically.
     * @return true if need refresh, or false;
     */
    bool needRefresh(const key_type& key, const std::time_t& timestamp)
    {
        bool check = refreshAll_ || key.isRandomRank;

        // check "_ctr" sort property
        if(!check)
        {
            const std::vector<std::pair<std::string , bool> >& sortProperties = key.sortInfo;
            std::vector<std::pair<std::string , bool> >::const_iterator cit;
            for (cit = sortProperties.begin(); cit != sortProperties.end(); cit++)
            {
                if (cit->first == faceted::CTRManager::kCtrPropName)
                {
                    check = true;
                    break;
                }
            }
            ///Fixme!
            ///COUNT(Property) will be refreshed from search cache
            ///This is a temporary solution before we exactly know the rule for evicting search
            ///cache periodically
            if(key.counterList.size() > 0 ) check = true;
        }
        if (check)
            return (std::time(NULL) - timestamp) > refreshInterval_;
        else
            return false;
    }

    // for the search which may consume a lot time, we treat it as special and cache in the special cache.
    bool IsSpecialSearch(const key_type& key)
    {
        return key.query == "*";
    }

private:

    cache_type cache_;
    cache_type special_cache_;
    time_t refreshInterval_; // seconds
    bool refreshAll_;
};

} // namespace sf1r

#endif // CORE_SEARCH_MANAGER_SEARCH_CACHE_H

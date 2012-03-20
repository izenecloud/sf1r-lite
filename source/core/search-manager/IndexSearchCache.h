#ifndef CORE_SEARCH_MANAGER_INDEXSEARCH_CACHE_H
#define CORE_SEARCH_MANAGER_INDEXSEARCH_CACHE_H
/**
 * @file core/search-manager/IndexSearchCache.h
 * @author Ian Yang
 * @date Created <2009-09-30 09:31:29>
 * @date Updated <2010-03-24 15:43:04>
 */
#include <query-manager/QueryIdentity.h>

#include <cache/IzeneCache.h>

#include <vector>
#include <deque>

namespace sf1r
{

class IndexSearchCache
{
public:
    struct value_type
    {
        std::time_t timestamp;
        std::vector<float> scores;
        std::vector<float> customScores;
        std::vector<unsigned int> docIdList;
        std::size_t totalCount;
        sf1r::PropertyRange propertyRange;
        std::size_t pageCount;
        std::vector<std::vector<izenelib::util::UString> > propertyQueryTermList;
    };
    typedef PureQueryIdentity key_type;

    explicit IndexSearchCache(unsigned cacheSize)
        : cache_(cacheSize)
        , refreshInterval_(60 * 60)
    {}

    /**
     * Cannot be \c const becuase \c IzeneCache::getValue is not const.
     */
    bool get(const key_type& key,
             std::vector<float>& scores,
             std::vector<float>& customScores,
             std::vector<unsigned int>& docIdList,
             std::size_t& totalCount,
             sf1r::PropertyRange& propertyRange,
             std::size_t* pageCount = NULL,
             std::vector<std::vector<izenelib::util::UString> >* propertyQueryTermList = NULL)
    {
        value_type value;
        if (cache_.getValueNoInsert(key, value))
        {
            scores.swap(value.scores);
            customScores.swap(value.customScores);
            docIdList.swap(value.docIdList);
            totalCount = value.totalCount;
            propertyRange.swap(value.propertyRange);
            if (pageCount)
                *pageCount = value.pageCount;
            if (propertyQueryTermList)
                propertyQueryTermList->swap(value.propertyQueryTermList);

            std::time_t timestamp = value.timestamp;
            if (needRefresh(key, timestamp))
                return false;
            else
                return true;
        }

        return false;
    }

    void set(const key_type& key,
             std::vector<float> scores,
             std::vector<float> customScores,
             std::vector<unsigned int> docIdList,
             std::size_t totalCount,
             sf1r::PropertyRange propertyRange,
             std::size_t pageCount = 0,
             std::vector<std::vector<izenelib::util::UString> >* propertyQueryTermList = NULL)
    {
        value_type value;
        value.timestamp = std::time(NULL);
        scores.swap(value.scores);
        customScores.swap(value.customScores);
        docIdList.swap(value.docIdList);
        value.totalCount = totalCount;
        propertyRange.swap(value.propertyRange);
        if (pageCount)
            value.pageCount = pageCount;
        if (propertyQueryTermList)
            value.propertyQueryTermList = *propertyQueryTermList;
        cache_.updateValue(key, value);
    }

    void clear()
    {
        cache_.clear();
    }

private:
    /**
     * For keys with static values, we need not to refresh cache;
     * but for those with volatile values, we need to refresh cache periodically.
     * @return true if need refresh, or false;
     */
    bool needRefresh(const key_type& key, const std::time_t& timestamp)
    {
        bool check = false;

        // check "_ctr" sort property
        const std::vector<std::pair<std::string , bool> >& sortProperties = key.sortInfo;
        std::vector<std::pair<std::string , bool> >::const_iterator cit;
        for (cit = sortProperties.begin(); cit != sortProperties.end(); cit++)
        {
            if (cit->first == "_ctr")
            {
                check = true;
                break;
            }
        }

        if (check){
            return (std::time(NULL) - timestamp) > refreshInterval_;
        }
        else
            return false;
    }

private:
    typedef izenelib::cache::IzeneCache<
        key_type,
        value_type,
        izenelib::util::ReadWriteLock,
        izenelib::cache::RDE_HASH,
        izenelib::cache::LRLFU
    > cache_type;

    cache_type cache_;
    time_t refreshInterval_; // seconds
};

} // namespace sf1r

#endif // CORE_SEARCH_MANAGER_INDEXSEARCH_CACHE_H

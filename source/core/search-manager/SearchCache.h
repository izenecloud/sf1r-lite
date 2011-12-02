#ifndef CORE_SEARCH_MANAGER_SEARCH_CACHE_H
#define CORE_SEARCH_MANAGER_SEARCH_CACHE_H
/**
 * @file core/search-manager/SearchCache.h
 * @author Ian Yang
 * @date Created <2009-09-30 09:31:29>
 * @date Updated <2010-03-24 15:43:04>
 */
#include <query-manager/QueryIdentity.h>
#include <mining-manager/faceted-submanager/ontology_rep.h>

#include <cache/IzeneCache.h>

#include <vector>
#include <deque>

namespace sf1r
{

class SearchCache
{
public:
    struct value_type
    {
        std::time_t timestamp;
        std::vector<float> scores;
        std::vector<float> customScores;
        std::vector<unsigned int> docIdList;
        std::size_t totalCount;
        faceted::GroupRep groupRep;
        faceted::OntologyRep attrRep;
        sf1r::PropertyRange propertyRange;
        DistKeywordSearchInfo distSearchInfo;
        std::size_t pageCount;
        std::vector<std::vector<izenelib::util::UString> > propertyQueryTermList;
        std::vector<uint32_t> workerIdList;
    };
    typedef QueryIdentity key_type;

    explicit SearchCache(unsigned cacheSize)
            : cache_(cacheSize)
            , refreshInterval_(60*60)
    {}

    /**
     * Cannot be \c const becuase \c IzeneCache::getValue is not const.
     */
    bool get(const key_type& key,
             std::vector<float>& scores,
             std::vector<float>& customScores,
             std::vector<unsigned int>& docIdList,
             std::size_t& totalCount,
             faceted::GroupRep& groupRep,
             faceted::OntologyRep& attrRep,
             sf1r::PropertyRange& propertyRange,
             DistKeywordSearchInfo& distSearchInfo,
             std::size_t* pageCount = NULL,
             std::vector<std::vector<izenelib::util::UString> >* propertyQueryTermList = NULL,
             std::vector<uint32_t>* workerIdList = NULL)
    {
        value_type value;
        if (cache_.getValueNoInsert(key, value))
        {
            scores.swap(value.scores);
            customScores.swap(value.customScores);
            docIdList.swap(value.docIdList);
            totalCount = value.totalCount;
            groupRep.swap(value.groupRep);
            attrRep.swap(value.attrRep);
            propertyRange.swap(value.propertyRange);
            distSearchInfo = value.distSearchInfo;
            if (pageCount)
                *pageCount = value.pageCount;
            if (propertyQueryTermList)
                (*propertyQueryTermList).swap(value.propertyQueryTermList);
            if (workerIdList)
                (*workerIdList).swap(value.workerIdList);

            std::time_t timestamp = value.timestamp;
            if (isNeedRefresh(key, timestamp))
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
             faceted::GroupRep& groupRep,
             faceted::OntologyRep attrRep,
             sf1r::PropertyRange propertyRange,
             DistKeywordSearchInfo distSearchInfo,
             std::size_t pageCount = 0,
             std::vector<std::vector<izenelib::util::UString> >* propertyQueryTermList = NULL,
             std::vector<uint32_t>* workerIdList = NULL)
    {
        if (distSearchInfo.nodeType_ == DistKeywordSearchInfo::NODE_WORKER)
            return;

        value_type value;
        value.timestamp = std::time(NULL);
        scores.swap(value.scores);
        customScores.swap(value.customScores);
        docIdList.swap(value.docIdList);
        value.totalCount = totalCount;
        groupRep.toOntologyRepItemList();
        value.groupRep = groupRep;
        attrRep.swap(value.attrRep);
        propertyRange.swap(value.propertyRange);
        value.distSearchInfo = distSearchInfo;
        if (pageCount)
            value.pageCount = pageCount;
        if (propertyQueryTermList)
            value.propertyQueryTermList = *propertyQueryTermList;
        if (workerIdList)
            value.workerIdList = *workerIdList;
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
    bool isNeedRefresh(const key_type& key, std::time_t timestamp)
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
    izenelib::cache::LFU
    > cache_type;

    cache_type cache_;
    time_t refreshInterval_; // seconds
};

} // namespace sf1r

#endif // CORE_SEARCH_MANAGER_SEARCH_CACHE_H

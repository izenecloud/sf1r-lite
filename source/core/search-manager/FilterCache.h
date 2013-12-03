#ifndef CORE_SEARCH_MANAGER_FILTER_CACHE_H
#define CORE_SEARCH_MANAGER_FILTER_CACHE_H
/**
 * @file core/search-manager/FilterCache.h
 * @author Yingfeng
 * @date Created <2010-04-09 16:12:00>
 */

#include <query-manager/ActionItem.h>
#include <index-manager/InvertedIndexManager.h>
#include <cache/IzeneCache.h>

#include <boost/shared_ptr.hpp>

namespace sf1r
{
class FilterCache
{
public:
    typedef QueryFiltering::FilteringType key_type;
    typedef boost::shared_ptr<InvertedIndexManager::FilterBitmapT> value_type;

public:
    explicit FilterCache(unsigned cacheSize)
        : cache_(cacheSize)
    {}

    ~FilterCache()
    {
        clear();
    }

    bool get(const key_type& key, value_type& value)
    {
        if (cache_.getValueNoInsert(key, value))
            return true;

        return false;
    }

    void set(const key_type& key, value_type value)
    {
        cache_.insertValue(key, value);
    }

    void clear()
    {
        cache_.clear();
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
};

} // namespace sf1r

#endif // CORE_SEARCH_MANAGER_FILTER_CACHE_H

#ifndef CORE_SEARCH_MANAGER_FILTER_CACHE_H
#define CORE_SEARCH_MANAGER_FILTER_CACHE_H
/**
 * @file core/search-manager/FilterCache.h
 * @author Yingfeng
 * @date Created <2010-04-09 16:12:00>
 */

#include <query-manager/ActionItem.h>
#include <ir/index_manager/utility/Bitset.h>
#include <am/bitmap/ewah.h>
#include <cache/IzeneCache.h>

#include <boost/shared_ptr.hpp>
#include <boost/variant/variant.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/apply_visitor.hpp>

namespace sf1r
{

class FilterCache
{
public:
    typedef QueryFiltering::FilteringType key_type;

    typedef izenelib::ir::indexmanager::Bitset RawBitmap;
    typedef boost::shared_ptr<RawBitmap> RawBitmapPointer;

    typedef izenelib::ir::indexmanager::EWAHBoolArray<uint64_t> CompressBitmap;
    typedef boost::shared_ptr<CompressBitmap> CompressBitmapPointer;

    typedef boost::variant<RawBitmapPointer, CompressBitmapPointer> value_type;

public:
    explicit FilterCache(unsigned cacheSize)
            : visitor_()
            , cache_(cacheSize)
    {}

    ~FilterCache()
    {
        clear();
    }

    bool get(const key_type& key, RawBitmapPointer& raw)
    {
        value_type cache_value;
        if (!cache_.getValueNoInsert(key, cache_value))
            return false;

        raw = boost::apply_visitor(visitor_, cache_value);
        return true;
    }

    void set(const key_type& key, RawBitmapPointer raw)
    {
        float density = raw->count();
        density /= raw->size();

        if (density > 0.02)
        {
            cache_.insertValue(key, raw);
        }
        else
        {
            CompressBitmapPointer compress(new CompressBitmap);
            raw->compress(*compress);
            cache_.insertValue(key, compress);
        }
    }

    void clear()
    {
        cache_.clear();
    }

private:

    class FilterCacheValueVisitor : public boost::static_visitor<RawBitmapPointer>
    {
      public:
        RawBitmapPointer operator()(RawBitmapPointer& raw) const
        {
            return raw;
        }

        RawBitmapPointer operator()(CompressBitmapPointer& compress) const
        {
            RawBitmapPointer raw(new RawBitmap);
            raw->decompress(*compress);
            return raw;
        }

    };

    const FilterCacheValueVisitor visitor_;

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

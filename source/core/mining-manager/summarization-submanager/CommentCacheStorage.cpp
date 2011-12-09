#include "CommentCacheStorage.h"

namespace sf1r
{

CommentCacheStorage::CommentCacheStorage(
        const std::string& dbPath,
        uint32_t buffer_capacity)
    : comment_cache_db_(dbPath)
    , buffer_capacity_(buffer_capacity)
{
    if (!comment_cache_db_.open())
    {
        boost::filesystem::remove_all(dbPath);
        comment_cache_db_.open();
    }
}

CommentCacheStorage::~CommentCacheStorage()
{
}

void CommentCacheStorage::AppendUpdate(const UString& key, uint32_t docid, const UString& content)
{
    buffer_db_[key].second.push_back(std::make_pair(docid, content));

    ++buffer_size_;
    if (IsBufferFull_())
        Flush();
}

void CommentCacheStorage::Delete(const UString& key)
{
    buffer_db_[key].first = true;

    ++buffer_size_;
    if (IsBufferFull_())
        Flush();
}

void CommentCacheStorage::Flush()
{
    if (buffer_db_.empty()) return;

    for (BufferType::iterator it = buffer_db_.begin();
            it != buffer_db_.end(); ++it)
    {
        if (it->second.first)
        {
            comment_cache_db_.del(it->first);
            continue;
        }

        CommentCacheItemType value;
        comment_cache_db_.get(it->first, value);
        value.second.reserve(value.second.size() + it->second.second.size());
        bool dirty = false;
        std::vector<std::pair<uint32_t, UString> >::iterator vit
            = it->second.second.begin();
        for (; vit != it->second.second.end(); ++vit)
        {
            if (value.first.insert(vit->first).second)
            {
                dirty = true;
                value.second.push_back(UString());
                value.second.back().swap(vit->second);
            }
        }
        if (dirty)
            comment_cache_db_.update(it->first, value);
    }

    comment_cache_db_.flush();
    buffer_db_.clear();
    buffer_size_ = 0;
}

bool CommentCacheStorage::Get(const UString& key, CommentCacheItemType& result)
{
    return comment_cache_db_.get(key, result);
}

}

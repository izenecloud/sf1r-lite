#include "CommentCacheStorage.h"

namespace
{
static const std::string comment_cache_path("/comment_cache");
static const std::string dirty_key_path("/dirty_key");
}

namespace sf1r
{

CommentCacheStorage::CommentCacheStorage(
        const std::string& dbPath,
        uint32_t buffer_capacity)
    : comment_cache_db_(dbPath + comment_cache_path)
    , dirty_key_db_(dbPath + dirty_key_path)
    , buffer_capacity_(buffer_capacity)
    , buffer_size_(0)
{
    if (!comment_cache_db_.open())
    {
        boost::filesystem::remove_all(dbPath + comment_cache_path);
        comment_cache_db_.open();
    }
    if (!dirty_key_db_.open())
    {
        boost::filesystem::remove_all(dbPath + dirty_key_path);
        dirty_key_db_.open();
    }
}

CommentCacheStorage::~CommentCacheStorage()
{
}

void CommentCacheStorage::AppendUpdate(const KeyType& key, uint32_t docid, const ContentType& content)
{
    buffer_db_[key].second.push_back(std::make_pair(docid, content));

    ++buffer_size_;
    if (IsBufferFull_())
        Flush();
}

void CommentCacheStorage::Delete(const KeyType& key)
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
        std::vector<std::pair<uint32_t, ContentType> >::const_iterator vit
            = it->second.second.begin();
        for (; vit != it->second.second.end(); ++vit)
        {
            if (value.first.insert(vit->first).second)
            {
                dirty = true;
                value.second.push_back(vit->second);
            }
        }
        if (dirty)
        {
            comment_cache_db_.update(it->first, value);
            dirty_key_db_.update(it->first, 0);
        }
    }

    comment_cache_db_.flush();
    dirty_key_db_.flush();
    buffer_db_.clear();
    buffer_size_ = 0;
}

bool CommentCacheStorage::Get(const KeyType& key, CommentCacheItemType& result)
{
    return comment_cache_db_.get(key, result);
}

bool CommentCacheStorage::ClearDirtyKey()
{
    return dirty_key_db_.clear();
}

}

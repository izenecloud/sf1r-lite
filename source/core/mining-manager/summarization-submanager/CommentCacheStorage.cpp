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

void CommentCacheStorage::Insert(const UString& key, uint32_t docid, const UString& content)
{
    buffer_db_[key].push_back(std::make_pair(docid, content));

    ++buffer_size_;
    if (IsBufferFull_())
        Flush();
}

void CommentCacheStorage::Update(const UString& key, uint32_t docid, const UString& content)
{
    //TODO
}

void CommentCacheStorage::Delete(const UString& key, uint32_t docid)
{
    //TODO
}

void CommentCacheStorage::Flush()
{
    if (buffer_db_.empty()) return;

    BufferType::iterator it = buffer_db_.begin();
    for (; it != buffer_db_.end(); ++it)
    {
        CommentCacheItemType value;
        comment_cache_db_.get(it->first, value);
        value.second.reserve(value.second.size() + it->second.size());
        for (BufferType::data_type::iterator vit = it->second.begin();
                vit != it->second.end(); ++vit)
        {
            value.first.set(vit->first);
            value.second.push_back(UString());
            value.second.back().swap(vit->second);
        }
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

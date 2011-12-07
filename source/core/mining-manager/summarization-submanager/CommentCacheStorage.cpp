#include "CommentCacheStorage.h"
#include "Summarization.h"

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

void CommentCacheStorage::AppendUpdate(const UString& key, uint32_t first, const UString& second)
{
    buffer_db_[key].push_back(std::make_pair(first, second));

    ++buffer_size_;
    if (IsBufferFull_())
        Flush();
}

void CommentCacheStorage::Flush()
{
    if (buffer_db_.empty()) return;

    BufferType::iterator it = buffer_db_.begin();
    for (; it != buffer_db_.end(); ++it)
    {
//      CommentCacheItemType value;
//      if (comment_cache_db_.get(it->first, value))
//      {
//          for (CommentCacheItemType::iterator vit = it->second.begin();
//                  vit != it->second.end(); ++vit)
//          {
//              value.push_back(std::make_pair(vit->first, UString()));
//              value.back().second.swap(vit->second);
//          }
//          comment_cache_db_.update(it->first, value);
//      }
//      else
        {
            comment_cache_db_.update(it->first, it->second);
        }
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

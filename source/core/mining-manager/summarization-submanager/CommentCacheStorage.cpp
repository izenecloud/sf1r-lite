#include "CommentCacheStorage.h"
#include "Summarization.h"

namespace sf1r
{

CommentCacheStorage::CommentCacheStorage(
        const std::string& dbPath)
    : comment_cache_db_(dbPath)
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

void CommentCacheStorage::Update(const UString& key, const CommentCacheItemType& value)
{
    comment_cache_db_.update(key, value);
}

void CommentCacheStorage::Flush()
{
    comment_cache_db_.flush();
}

bool CommentCacheStorage::Get(const UString& key, CommentCacheItemType& result)
{
    return comment_cache_db_.get(key, result);
}

}

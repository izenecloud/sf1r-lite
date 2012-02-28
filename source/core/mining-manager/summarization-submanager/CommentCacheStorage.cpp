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
    : dirty_key_db_(dbPath + dirty_key_path)
    , dispatcher_(dirty_key_db_)
    , op_type_(CommentCacheStorage::NONE)
{
    if (!dirty_key_db_.open())
    {
        boost::filesystem::remove_all(dbPath + dirty_key_path);
        dirty_key_db_.open();
    }
    boost::filesystem::create_directories(dbPath + comment_cache_path);
    comment_cache_drum_.reset(new CommentCacheDrumType(dbPath + comment_cache_path, 64, 32768, 4194304, dispatcher_));
}

CommentCacheStorage::~CommentCacheStorage()
{
}

void CommentCacheStorage::AppendUpdate(const KeyType& key, uint32_t docid, const ContentType& content)
{
    if (op_type_ != APPEND_UPDATE)
    {
        Flush();
        op_type_ = APPEND_UPDATE;
    }

    CommentCacheItemType cache_item;
    cache_item[docid] = content;
    comment_cache_drum_->Append(key, cache_item);
}

void CommentCacheStorage::Delete(const KeyType& key)
{
    if (op_type_ != DELETE)
    {
        Flush();
        op_type_ = DELETE;
    }

    comment_cache_drum_->Delete(key);
}

void CommentCacheStorage::Flush()
{
    if (op_type_ == NONE) return;

    comment_cache_drum_->Synchronize();
    dirty_key_db_.flush();
    op_type_ = NONE;
}

bool CommentCacheStorage::Get(const KeyType& key, CommentCacheItemType& result)
{
    return comment_cache_drum_->GetValue(key, result);
}

bool CommentCacheStorage::ClearDirtyKey()
{
    return dirty_key_db_.clear();
}

}

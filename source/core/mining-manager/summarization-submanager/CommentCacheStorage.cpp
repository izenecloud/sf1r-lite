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
    , buffer_capacity_(buffer_capacity)
    , buffer_size_(0)
    , op_type_(CommentCacheStorage::NONE)
{
    if (!dirty_key_db_.open())
    {
        boost::filesystem::remove_all(dbPath + dirty_key_path);
        dirty_key_db_.open();
    }
    boost::filesystem::create_directories(dbPath + comment_cache_path);
    comment_cache_drum_.reset(new CommentCacheDrumType(dbPath + comment_cache_path, 64, 2048, 16777216, dispatcher_));
}

CommentCacheStorage::~CommentCacheStorage()
{
}

void CommentCacheStorage::AppendUpdate(const KeyType& key, uint32_t docid, const ContentType& content,
   const AdvantageType& advantage, const DisadvantageType& disadvantage, const ScoreType& score)
{
    if (op_type_ != APPEND_UPDATE)
    {
        Flush(true);
        op_type_ = APPEND_UPDATE;
    }

    CommentSummaryT cs;
    cs.get<0>() = content;
    cs.get<1>() = advantage;
    cs.get<2>() = disadvantage;
    cs.get<3>() = score;
    buffer_db_[key][docid] = cs;

    ++buffer_size_;
    if (IsBufferFull_())
        Flush();
}

void CommentCacheStorage::ExpelUpdate(const KeyType& key, uint32_t docid)
{
    if (op_type_ != EXPEL_UPDATE)
    {
        Flush(true);
        op_type_ = EXPEL_UPDATE;
    }

    buffer_db_[key][docid];

    ++buffer_size_;
    if (IsBufferFull_())
        Flush();
}

void CommentCacheStorage::Delete(const KeyType& key)
{
    if (op_type_ != DELETE)
    {
        Flush(true);
        op_type_ = DELETE;
    }

    buffer_db_[key];

    ++buffer_size_;
    if (IsBufferFull_())
        Flush();
}

void CommentCacheStorage::Flush(bool needSync)
{
    if (op_type_ == NONE) return;
    if (!needSync && buffer_db_.empty()) return;

    switch (op_type_)
    {
    case APPEND_UPDATE:
        for (BufferType::iterator it = buffer_db_.begin();
                it != buffer_db_.end(); ++it)
        {
            comment_cache_drum_->Append(it->first, it->second);
        }
        break;

    case EXPEL_UPDATE:
        for (BufferType::iterator it = buffer_db_.begin();
                it != buffer_db_.end(); ++it)
        {
            comment_cache_drum_->Expel(it->first, it->second);
        }
        break;

    case DELETE:
        for (BufferType::iterator it = buffer_db_.begin();
                it != buffer_db_.end(); ++it)
        {
            comment_cache_drum_->Delete(it->first);
        }
        break;

    default:
        break;
    }

    if (needSync)
    {
        comment_cache_drum_->Synchronize();
        dirty_key_db_.flush();
        op_type_ = NONE;
    }

    buffer_db_.clear();
    buffer_size_ = 0;
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

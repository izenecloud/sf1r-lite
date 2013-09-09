#include "CommentCacheStorage.h"

namespace
{
static const std::string comment_cache_path("/comment_cache");
static const std::string dirty_key_path("/dirty_key");
static const std::string recent_comment_key_path("/recent_comment_key");
}

namespace sf1r
{

CommentCacheStorage::CommentCacheStorage(
        const std::string& dbPath,
        uint32_t buffer_capacity)
    : dirty_key_db_(dbPath + dirty_key_path)
    , recent_comment_db_(dbPath + recent_comment_key_path)
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
    recent_comment_db_.open();
    boost::filesystem::create_directories(dbPath + comment_cache_path);
    comment_cache_drum_.reset(new CommentCacheDrumType(dbPath + comment_cache_path, 64, 2048, 16777216, dispatcher_));
}

CommentCacheStorage::~CommentCacheStorage()
{
}

void CommentCacheStorage::setRecentTime(int64_t timestamp)
{
    recent_timestamp_ = timestamp;
}

void CommentCacheStorage::AppendUpdate(const KeyType& key, uint32_t docid, const ContentType& content,
   const AdvantageType& advantage, const DisadvantageType& disadvantage, const ScoreType& score,
   int64_t timestamp)
{
    if (op_type_ != APPEND_UPDATE)
    {
        Flush(true);
        op_type_ = APPEND_UPDATE;
    }

    CommentSummaryT cs;
    cs.content = content;
    cs.advantage = advantage;
    cs.disadvantage = disadvantage;
    cs.score = score;
    cs.timestamp = timestamp;

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
            recent_comment_db_.del(it->first);
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

size_t CommentCacheStorage::setRecentComments(const KeyType& key, const CommentCacheItemType& comments)
{
    if (comments.empty())
    {
        recent_comment_db_.del(key);
        return 0;
    }
    RecentCommentItem recent_item;
    recent_item.oldest_time = comments.begin()->second.timestamp;
    recent_item.oldest_docid = comments.begin()->first;
    for (CommentCacheItemType::const_iterator item_it = comments.begin();
        item_it != comments.end(); ++item_it)
    {
        if (item_it->second.timestamp > recent_timestamp_)
        {
            recent_item.all_times[item_it->first] = item_it->second.timestamp; 
            if (item_it->second.timestamp < recent_item.oldest_time)
            {
                recent_item.oldest_time = item_it->second.timestamp;
                recent_item.oldest_docid = item_it->first;
            }
        }
    }
    recent_comment_db_.insert(key, recent_item);
    return recent_item.all_times.size();
}

void CommentCacheStorage::updateRecentComments()
{
    RecentCommentIteratorType it(recent_comment_db_);
    RecentCommentIteratorType it_end;
    std::vector<KeyType> update_keys;

    for( ; it != it_end; ++it)
    {
        if (it->second.oldest_time > recent_timestamp_)
        {
            // all comments is in recent days, no need to update.
            continue;
        }
        update_keys.push_back(it->first);
    }
    LOG(INFO) << "udpate recent comments num : " << update_keys.size();
    for(size_t i = 0; i < update_keys.size(); ++i)
    {
        const KeyType& key = update_keys[i];
        // clean the comments that are out of date.
        RecentCommentItem olditem;
        recent_comment_db_.get(key, olditem);
        if (olditem.all_times.empty())
        {
            recent_comment_db_.del(it->first);
            continue;
        }
        RecentCommentItem newitem;
        for(std::map<docid_t, int64_t>::const_iterator recent_it = olditem.all_times.begin();
            recent_it != olditem.all_times.end(); ++recent_it)
        {
            if (recent_it->second > recent_timestamp_ )
            {
                if (newitem.all_times.empty() || recent_it->second < newitem.oldest_time)
                {
                    newitem.oldest_time = recent_it->second;
                    newitem.oldest_docid = recent_it->first;
                }
                newitem.all_times[recent_it->first] = recent_it->second;
            }
        }
        if (newitem.all_times.empty())
        {
            recent_comment_db_.del(key);
            continue;
        }
        recent_comment_db_.insert(key, newitem);
    }
}

}

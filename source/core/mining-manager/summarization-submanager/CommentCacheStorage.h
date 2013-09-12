#ifndef SF1R_MINING_MANAGER_SUMMARIZATION_SUBMANAGER_COMMENT_CACHE_STORAGE_H
#define SF1R_MINING_MANAGER_SUMMARIZATION_SUBMANAGER_COMMENT_CACHE_STORAGE_H

#include "summarization_types.h"

#include <3rdparty/am/drum/drum.hpp>
#include <am/leveldb/Table.h>
#include <am/range/AmIterator.h>
#include <3rdparty/am/stx/btree_map.h>
#include <util/izene_serialization.h>

namespace sf1r
{

template <class key_t,
          class value_t,
          class aux_t>
class CommentCacheDispatcher
    : public izenelib::drum::NullDispatcher<key_t, value_t, aux_t>
{
    typedef izenelib::am::leveldb::Table<key_t, char> DirtyKeyDbType;

public:
    explicit CommentCacheDispatcher(DirtyKeyDbType& db)
        : db_(db)
        , prev_key_()
        , first_time_(true)
    {
    }

    void Delete(key_t const& key, aux_t const& aux) const
    {
        db_.update(key, 0);
    }

    void UniqueKeyAppend(key_t const& key, value_t const& value, aux_t const& aux) const
    {
        if (first_time_)
        {
            db_.update(key, 0);
            prev_key_ = key;
            first_time_ = false;
        }
        else if (key != prev_key_)
        {
            db_.update(key, 0);
            prev_key_ = key;
        }
    }

    void UniqueKeyExpel(key_t const& key, value_t const& value, aux_t const& aux) const
    {
        if (first_time_)
        {
            db_.update(key, 0);
            prev_key_ = key;
            first_time_ = false;
        }
        else if (key != prev_key_)
        {
            db_.update(key, 0);
            prev_key_ = key;
        }
    }

private:
    DirtyKeyDbType& db_;
    mutable key_t prev_key_;
    mutable bool first_time_;
};

struct RecentCommentItem
{
    int64_t oldest_time;
    docid_t  oldest_docid;
    std::map<docid_t, int64_t> all_times;
    
    template<class DataIO>
    friend void DataIO_loadObject(DataIO& dio, RecentCommentItem& x);
    template<class DataIO>
    friend void DataIO_saveObject(DataIO& dio, const RecentCommentItem& x);
};

template<class DataIO>
inline void DataIO_loadObject(DataIO& dio, RecentCommentItem& x)
{
    dio & x.oldest_time;
    dio & x.oldest_docid;
    dio & x.all_times;
}

template<class DataIO>
inline void DataIO_saveObject(DataIO& dio, const RecentCommentItem& x)
{
    dio & x.oldest_time;
    dio & x.oldest_docid;
    dio & x.all_times;
}

class CommentCacheStorage
{
public:
    typedef izenelib::drum::Drum<
        KeyType,
        CommentCacheItemType,
        char,
        izenelib::am::leveldb::TwoPartComparator,
        izenelib::am::leveldb::Table,
        CommentCacheDispatcher
    > CommentCacheDrumType;

    typedef izenelib::am::leveldb::Table<KeyType, char> DirtyKeyDbType;
    typedef izenelib::am::leveldb::Table<KeyType, RecentCommentItem> RecentCommentDbType;
    typedef izenelib::am::AMIterator<RecentCommentDbType> RecentCommentIteratorType;
    typedef izenelib::am::AMIterator<DirtyKeyDbType> DirtyKeyIteratorType;

    typedef stx::btree_map<KeyType, CommentCacheItemType> BufferType;

    enum OpType
    {
        NONE,
        APPEND_UPDATE,
        EXPEL_UPDATE,
        DELETE
    };

    CommentCacheStorage(
            const std::string& dbPath,
            uint32_t buffer_capacity = 20000);

    ~CommentCacheStorage();

    void AppendUpdate(const KeyType& key, uint32_t docid, const ContentType& content,
       const AdvantageType& advantage, const DisadvantageType& disadvantage,
       const ScoreType& score, int64_t timestamp);

    void ExpelUpdate(const KeyType& key, uint32_t docid);

    void Delete(const KeyType& key);

    void Flush(bool needSync = false);

    bool Get(const KeyType& key, CommentCacheItemType& value);

    bool ClearDirtyKey();

    void setRecentTime(int64_t timestamp);
    void updateRecentComments(std::vector<KeyType>& update_keys,
        std::vector<uint32_t>& recent_comment_count_list);
    size_t setRecentComments(const KeyType& key, const CommentCacheItemType& comments);

private:
    inline bool IsBufferFull_()
    {
        return buffer_size_ >= buffer_capacity_;
    }

private:
    friend class MultiDocSummarizationSubManager;

    DirtyKeyDbType dirty_key_db_;
    RecentCommentDbType recent_comment_db_;
    int64_t  recent_timestamp_;
    CommentCacheDispatcher<KeyType, CommentCacheItemType, char> dispatcher_;
    boost::shared_ptr<CommentCacheDrumType> comment_cache_drum_;

    BufferType buffer_db_;

    uint32_t buffer_capacity_;
    uint32_t buffer_size_;

    OpType op_type_;
};

}

MAKE_FEBIRD_SERIALIZATION(sf1r::RecentCommentItem)

#endif

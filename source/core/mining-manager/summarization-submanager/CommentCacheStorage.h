#ifndef SF1R_MINING_MANAGER_SUMMARIZATION_SUBMANAGER_COMMENT_CACHE_STORAGE_H
#define SF1R_MINING_MANAGER_SUMMARIZATION_SUBMANAGER_COMMENT_CACHE_STORAGE_H

#include <util/ustring/UString.h>
#include <3rdparty/am/drum/drum.hpp>
#include <am/leveldb/Table.h>
#include <am/range/AmIterator.h>
#include <3rdparty/am/stx/btree_map.h>
#include <util/izene_serialization.h>

#define USE_LOG_SERVER

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
    {
    }

    virtual void Delete(key_t const& key, aux_t const& aux) const
    {
        db_.update(key, 0);
    }

    virtual void UniqueKeyAppend(key_t const& key, value_t const& value, aux_t const& aux) const
    {
        db_.update(key, 0);
    }

    virtual void DuplicateKeyAppend(key_t const& key, value_t const& value, aux_t const& aux) const
    {
        db_.update(key, 0);
    }

private:
    DirtyKeyDbType& db_;
};

class CommentCacheAppender
{
public:
    template <class T1, class T2>
    bool operator() (std::pair<std::set<T1>, std::vector<T2> >& left, const std::pair<std::set<T1>, std::vector<T2> >& right) const
    {
        std::size_t old_size = left.first.size();
        left.first.insert(right.first.begin(), right.first.end());

        if (left.first.size() == old_size)
            return false;

        left.second.insert(left.second.end(), right.second.begin(), right.second.end());
        return true;
    }
};

class CommentCacheStorage
{
public:
#ifdef USE_LOG_SERVER
    typedef uint128_t KeyType;
#else
    typedef izenelib::util::UString KeyType;
#endif
    typedef izenelib::util::UString ContentType;

    typedef std::pair<std::set<uint32_t>, std::vector<ContentType> > CommentCacheItemType;
    typedef izenelib::drum::Drum<
        KeyType,
        CommentCacheItemType,
        char,
        izenelib::am::leveldb::TwoPartComparator,
        izenelib::am::leveldb::Table,
        CommentCacheDispatcher,
        CommentCacheAppender
    > CommentCacheDrumType;

    typedef izenelib::am::leveldb::Table<KeyType, char> DirtyKeyDbType;
    typedef izenelib::am::AMIterator<DirtyKeyDbType> DirtyKeyIteratorType;

    typedef stx::btree_map<KeyType, CommentCacheItemType> BufferType;

    enum OpType
    {
        NONE,
        APPEND_UPDATE,
        DELETE
    };

    CommentCacheStorage(
            const std::string& dbPath,
            uint32_t buffer_capacity = 5000);

    ~CommentCacheStorage();

    void AppendUpdate(const KeyType& key, uint32_t docid, const ContentType& content);

    void Delete(const KeyType& key);

    void Flush(bool needSync = false);

    bool Get(const KeyType& key, CommentCacheItemType& value);

    bool ClearDirtyKey();

private:
    inline bool IsBufferFull_()
    {
        return buffer_size_ >= buffer_capacity_;
    }

private:
    friend class MultiDocSummarizationSubManager;

    DirtyKeyDbType dirty_key_db_;
    CommentCacheDispatcher<KeyType, CommentCacheItemType, char> dispatcher_;
    boost::shared_ptr<CommentCacheDrumType> comment_cache_drum_;

    BufferType buffer_db_;

    uint32_t buffer_capacity_;
    uint32_t buffer_size_;

    OpType op_type_;
};

}

MAKE_FEBIRD_SERIALIZATION(sf1r::CommentCacheStorage::CommentCacheItemType)

#endif

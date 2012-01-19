#ifndef SF1R_MINING_MANAGER_SUMMARIZATION_SUBMANAGER_COMMENT_CACHE_STORAGE_H
#define SF1R_MINING_MANAGER_SUMMARIZATION_SUBMANAGER_COMMENT_CACHE_STORAGE_H

#include <util/ustring/UString.h>
#include <am/leveldb/Table.h>
#include <am/tokyo_cabinet/tc_fixdb.h>
#include <am/range/AmIterator.h>
#include <3rdparty/am/stx/btree_map.h>
#include <util/izene_serialization.h>

//#define USE_LOG_SERVER

namespace sf1r
{

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
    typedef izenelib::am::leveldb::Table<KeyType, CommentCacheItemType> CommentCacheDbType;
    typedef izenelib::am::AMIterator<CommentCacheDbType> CommentCacheIteratorType;

    typedef izenelib::am::tc_fixdb<KeyType> DirtyKeyDbType;
    typedef izenelib::am::AMIterator<DirtyKeyDbType> DirtyKeyIteratorType;

    typedef stx::btree_map<KeyType, std::pair<bool, std::vector<std::pair<uint32_t, ContentType> > > > BufferType;

    CommentCacheStorage(
            const std::string& dbPath,
            uint32_t buffer_capacity = 5000);

    ~CommentCacheStorage();

    void AppendUpdate(const KeyType& key, uint32_t docid, const ContentType& content);

    void Delete(const KeyType& key);

    void Flush();

    bool GetCommentCache(const KeyType& key, CommentCacheItemType& value);

    bool GetDirtyKey(int32_t index, KeyType& dirty_key);

    bool ClearDirtyKey();

    inline int32_t getDirtyKeyCount() const
    {
        return dirty_key_count_;
    }

private:
    inline bool IsBufferFull_()
    {
        return buffer_size_ >= buffer_capacity_;
    }

private:
    friend class MultiDocSummarizationSubManager;

    CommentCacheDbType comment_cache_db_;
    DirtyKeyDbType dirty_key_db_;

    BufferType buffer_db_;

    uint32_t buffer_capacity_;
    uint32_t buffer_size_;
    int32_t dirty_key_count_;
};

}

MAKE_FEBIRD_SERIALIZATION(sf1r::CommentCacheStorage::CommentCacheItemType)

#endif

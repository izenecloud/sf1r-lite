#ifndef SF1R_MINING_MANAGER_SUMMARIZATION_SUBMANAGER_COMMENT_CACHE_STORAGE_H
#define SF1R_MINING_MANAGER_SUMMARIZATION_SUBMANAGER_COMMENT_CACHE_STORAGE_H

#include <util/ustring/UString.h>
#include <am/leveldb/Table.h>
#include <am/range/AmIterator.h>
#include <am/bitmap/Ewah.h>
#include <3rdparty/am/stx/btree_map.h>

namespace sf1r
{

using izenelib::util::UString;

class Summarization;

class CommentCacheStorage
{
    typedef std::vector<std::pair<uint32_t, UString> > CommentCacheItemType;
    typedef izenelib::am::leveldb::Table<UString, CommentCacheItemType> CommentCacheDbType;
    typedef izenelib::am::AMIterator<CommentCacheDbType> CommentCacheIteratorType;
    typedef stx::btree_map<UString, CommentCacheItemType> BufferType;

public:
    CommentCacheStorage(
            const std::string& dbPath,
            uint32_t buffer_capacity = 5000);

    ~CommentCacheStorage();

    void AppendUpdate(const UString& key, uint32_t first, const UString& second);

    void Flush();

    bool Get(const UString& key, CommentCacheItemType& value);

private:
    inline bool IsBufferFull_()
    {
        return buffer_size_ >= buffer_capacity_;
    }

private:
    friend class MultiDocSummarizationSubManager;

    CommentCacheDbType comment_cache_db_;

    BufferType buffer_db_;

    unsigned int buffer_capacity_;
    unsigned int buffer_size_;
};

}

#endif

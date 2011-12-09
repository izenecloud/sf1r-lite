#ifndef SF1R_MINING_MANAGER_SUMMARIZATION_SUBMANAGER_COMMENT_CACHE_STORAGE_H
#define SF1R_MINING_MANAGER_SUMMARIZATION_SUBMANAGER_COMMENT_CACHE_STORAGE_H

#include <util/ustring/UString.h>
#include <am/leveldb/Table.h>
#include <am/range/AmIterator.h>
#include <am/bitmap/Ewah.h>
#include <3rdparty/am/stx/btree_map.h>
#include <util/izene_serialization.h>

namespace izenelib
{
namespace util
{

template <>
struct IsFebirdSerial<std::pair<izenelib::am::EWAHBoolArray<uint32_t>, std::vector<izenelib::util::UString> > >
{
    enum
    {
        yes = 1,
        no = !yes
    };
};

}
}

namespace sf1r
{

using izenelib::util::UString;

class CommentCacheStorage
{
    typedef std::pair<izenelib::am::EWAHBoolArray<uint32_t>, std::vector<UString> > CommentCacheItemType;
    typedef izenelib::am::leveldb::Table<UString, CommentCacheItemType> CommentCacheDbType;
    typedef izenelib::am::AMIterator<CommentCacheDbType> CommentCacheIteratorType;
    typedef stx::btree_map<UString, std::vector<std::pair<uint32_t, UString> > > BufferType;

public:
    CommentCacheStorage(
            const std::string& dbPath,
            uint32_t buffer_capacity = 5000);

    ~CommentCacheStorage();

    void Insert(const UString& key, uint32_t docid, const UString& content);

    void Update(const UString& key, uint32_t docid, const UString& content);

    void Delete(const UString& key, uint32_t docid);

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

#ifndef SF1R_MINING_MANAGER_SUMMARIZATION_SUBMANAGER_COMMENT_CACHE_STORAGE_H
#define SF1R_MINING_MANAGER_SUMMARIZATION_SUBMANAGER_COMMENT_CACHE_STORAGE_H

#include <util/ustring/UString.h>
#include <am/leveldb/Table.h>
#include <am/range/AmIterator.h>
#include <am/bitmap/Ewah.h>

#include <vector>

namespace sf1r
{

using izenelib::util::UString;

class Summarization;

class CommentCacheStorage
{
    typedef std::vector<std::pair<uint32_t, UString> > CommentCacheItemType;
    typedef izenelib::am::leveldb::Table<UString, CommentCacheItemType> CommentCacheDbType;
    typedef izenelib::am::AMIterator<CommentCacheDbType> CommentCacheIteratorType;

public:
    CommentCacheStorage(
            const std::string& dbPath);

    ~CommentCacheStorage();

    void Update(const UString& key, const CommentCacheItemType& value);

    bool Get(const UString& key, CommentCacheItemType& value);

    void Flush();

private:
    friend class MultiDocSummarizationSubManager;
    CommentCacheDbType comment_cache_db_;
};

}

#endif

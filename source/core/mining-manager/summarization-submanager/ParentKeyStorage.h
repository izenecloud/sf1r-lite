#ifndef SF1R_MINING_MANAGER_SUMMARIZATION_SUBMANAGER_PARENT_KEY_STORAGE_H
#define SF1R_MINING_MANAGER_SUMMARIZATION_SUBMANAGER_PARENT_KEY_STORAGE_H

#include <am/leveldb/Table.h>
#include <am/range/AmIterator.h>
#include <util/ustring/UString.h>
#include <util/izene_serialization.h>

#include <3rdparty/am/stx/btree_map.h>

#include <boost/thread.hpp>

namespace sf1r
{

class CommentCacheStorage;

class ParentKeyStorage
{
    typedef uint128_t ParentKeyType;
    typedef uint128_t ChildKeyType;

public:
    ParentKeyStorage(
            const std::string& db_dir,
            CommentCacheStorage* comment_cache_storage,
            unsigned bufferSize = 20000);

    ~ParentKeyStorage();

    bool GetChildren(const ParentKeyType& parent, std::vector<ChildKeyType>& children);

    bool GetParent(const ChildKeyType& child, ParentKeyType& parent);

private:
    CommentCacheStorage* comment_cache_storage_;

    boost::shared_mutex mutex_;
};

}

#endif

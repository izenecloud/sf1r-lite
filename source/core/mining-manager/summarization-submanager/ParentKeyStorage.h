#ifndef SF1R_MINING_MANAGER_SUMMARIZATION_SUBMANAGER_PARENT_KEY_STORAGE_H
#define SF1R_MINING_MANAGER_SUMMARIZATION_SUBMANAGER_PARENT_KEY_STORAGE_H

#include <am/leveldb/Table.h>
#include <am/range/AmIterator.h>
#include <util/ustring/UString.h>
#include <util/izene_serialization.h>

#include <3rdparty/am/stx/btree_map.h>

#include <boost/thread.hpp>

#define USE_LOG_SERVER

namespace sf1r
{

class CommentCacheStorage;

class ParentKeyStorage
{
#ifdef USE_LOG_SERVER
    typedef uint128_t ParentKeyType;
    typedef uint128_t ChildKeyType;
#else
    typedef izenelib::util::UString ParentKeyType;
    typedef izenelib::util::UString ChildKeyType;

    typedef izenelib::am::leveldb::Table<ParentKeyType, std::vector<ChildKeyType> > P2CDbType;
    typedef izenelib::am::AMIterator<P2CDbType> P2CIteratorType;

    typedef izenelib::am::leveldb::Table<ChildKeyType, ParentKeyType> C2PDbType;
    typedef izenelib::am::AMIterator<C2PDbType> C2PIteratorType;

    typedef stx::btree_map<ParentKeyType, std::pair<std::vector<ChildKeyType>, std::vector<ChildKeyType> > > BufferType;
#endif

public:
    ParentKeyStorage(
            const std::string& db_dir,
            CommentCacheStorage* comment_cache_storage,
            unsigned bufferSize = 20000);

    ~ParentKeyStorage();

    bool GetChildren(const ParentKeyType& parent, std::vector<ChildKeyType>& children);

    bool GetParent(const ChildKeyType& child, ParentKeyType& parent);

#ifndef USE_LOG_SERVER
    void Flush();

    void Insert(const ParentKeyType& parent, const ChildKeyType& child);

    void Update(const ParentKeyType& parent, const ChildKeyType& child);

    void Delete(const ParentKeyType& parent, const ChildKeyType& child);

private:
    inline bool IsBufferFull_()
    {
        return buffer_size_ >= buffer_capacity_;
    }
#endif

private:
    CommentCacheStorage* comment_cache_storage_;

#ifndef USE_LOG_SERVER
    P2CDbType parent_to_children_db_;
    C2PDbType child_to_parent_db_;

    BufferType buffer_db_;

    unsigned int buffer_capacity_;
    unsigned int buffer_size_;
#endif

    boost::shared_mutex mutex_;
};

}

#endif

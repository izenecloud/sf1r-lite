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

using izenelib::util::UString;

class ParentKeyStorage
{
    typedef izenelib::am::leveldb::Table<UString, std::vector<UString> > P2CDbType;
    typedef izenelib::am::AMIterator<P2CDbType> P2CIteratorType;

    typedef izenelib::am::leveldb::Table<UString, UString> C2PDbType;
    typedef izenelib::am::AMIterator<C2PDbType> C2PIteratorType;

    typedef stx::btree_map<UString, std::pair<std::vector<UString>, std::vector<UString> > > BufferType;

public:
    ParentKeyStorage(
            const std::string& db_dir,
            unsigned bufferSize = 20000);

    ~ParentKeyStorage();

    void Insert(const UString& parent, const UString& child);

    void Update(const UString& parent, const UString& child);

    void Delete(const UString& parent, const UString& child);

    void Flush();

    bool GetChildren(const UString& parent, std::vector<UString>& children);

    bool GetParent(const UString& child, UString& parent);

private:
    inline bool IsBufferFull_()
    {
        return buffer_size_ >= buffer_capacity_;
    }

private:
    friend class MultiDocSummarizationSubManager;

    static const std::string p2c_path;
    static const std::string c2p_path;

    P2CDbType parent_to_children_db_;
    C2PDbType child_to_parent_db_;

    BufferType buffer_db_;

    unsigned int buffer_capacity_;
    unsigned int buffer_size_;

    boost::shared_mutex mutex_;
};

}

#endif

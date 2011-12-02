#ifndef SF1R_MINING_MANAGER_SUMMARIZATION_SUBMANAGER_PARENT_KEY_STORAGE_H
#define SF1R_MINING_MANAGER_SUMMARIZATION_SUBMANAGER_PARENT_KEY_STORAGE_H

#include <am/leveldb/Table.h>
#include <util/ustring/UString.h>

#include <3rdparty/am/stx/btree_map.h>

namespace sf1r
{

using izenelib::util::UString;

class ParentKeyStorage
{
    typedef izenelib::am::leveldb::Table<UString, UString> ParentKeyDbType;
    typedef stx::btree_map<UString, UString> BufferType;

public:
    ParentKeyStorage(
        const std::string& dbPath,
        unsigned bufferSize = 20000);

    ~ParentKeyStorage();

    void AppendUpdate(const UString& key, const UString& value);

    void Flush();

private:
    inline bool IsBufferFull_()
    {
        return buffer_size_ >= buffer_capacity_;
    }

private:
    friend class MultiDocSummarizationSubManager;
    ParentKeyDbType parent_key_db_;
    BufferType buffer_db_;
    unsigned int buffer_capacity_;
    unsigned int buffer_size_;
    UString delimit_;
};

}

#endif

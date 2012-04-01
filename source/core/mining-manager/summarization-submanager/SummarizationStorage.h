#ifndef SF1R_MINING_MANAGER_SUMMARIZATION_SUBMANAGER_SUMMARIZATION_STORAGE_H
#define SF1R_MINING_MANAGER_SUMMARIZATION_SUBMANAGER_SUMMARIZATION_STORAGE_H

#include <am/leveldb/Table.h>

#include "Summarization.h"

#include <boost/thread.hpp>

#include <vector>

namespace sf1r
{

class SummarizationStorage
{
    typedef uint128_t KeyType;
    typedef izenelib::am::leveldb::Table<KeyType, Summarization> SummarizationDBType;

public:
    SummarizationStorage(
            const std::string& dbPath);

    ~SummarizationStorage();

    void Update(const KeyType& key, const Summarization& value);

    void Delete(const KeyType& key);

    void Flush();

    bool Get(const KeyType& key, Summarization& result);

    bool IsRebuildSummarizeRequired(
            const KeyType& key,
            const Summarization& value);

private:
    SummarizationDBType summarization_db_;
};

}

#endif

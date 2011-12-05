#ifndef SF1R_MINING_MANAGER_SUMMARIZATION_SUBMANAGER_SUMMARIZATION_STORAGE_H
#define SF1R_MINING_MANAGER_SUMMARIZATION_SUBMANAGER_SUMMARIZATION_STORAGE_H

#include <am/leveldb/Table.h>

#include "Summarization.h"

#include <boost/thread.hpp>

#include <vector>

namespace sf1r
{

using izenelib::util::UString;

class SummarizationStorage
{
    typedef izenelib::am::leveldb::Table<UString, Summarization> SummarizationDBType;

public:
    SummarizationStorage(
            const std::string& dbPath);

    ~SummarizationStorage();

    void Update(const UString& key, const Summarization& value);

    void Flush();

    bool Get(const UString& key, Summarization& result);

    bool IsRebuildSummarizeRequired(
            const UString& key,
            const Summarization& value);

private:
    SummarizationDBType summarization_db_;
};

}

#endif

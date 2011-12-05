#include "SummarizationStorage.h"

#include <iostream>

namespace sf1r
{

SummarizationStorage::SummarizationStorage(
        const std::string& dbPath)
    : summarization_db_(dbPath)
{
}

SummarizationStorage::~SummarizationStorage()
{
}

void SummarizationStorage::Update(const UString& key, const Summarization& value)
{
    summarization_db_.update(key,value);
}

void SummarizationStorage::Flush()
{
    summarization_db_.flush();
}

bool SummarizationStorage::Get(const UString& key, Summarization& result)
{
    return summarization_db_.get(key, result);
}

bool SummarizationStorage::IsRebuildSummarizeRequired(
        const UString& key,
        const Summarization& value)
{
    Summarization summarization;
    if(summarization_db_.get(key,summarization))
    {
        return summarization != value;
    }
    else
    {
        return true;
    }
}

}

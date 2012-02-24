#include "SummarizationStorage.h"

#include <iostream>

namespace
{
static const std::string summ_path("/summarization");
}

namespace sf1r
{

SummarizationStorage::SummarizationStorage(
        const std::string& dbPath)
    : summarization_db_(dbPath + summ_path)
{
    if (!summarization_db_.open())
    {
        boost::filesystem::remove_all(dbPath + summ_path);
        summarization_db_.open();
    }
}

SummarizationStorage::~SummarizationStorage()
{
}

void SummarizationStorage::Update(const KeyType& key, const Summarization& value)
{
    summarization_db_.update(key, value);
}

void SummarizationStorage::Delete(const KeyType& key)
{
    summarization_db_.del(key);
}

void SummarizationStorage::Flush()
{
    summarization_db_.flush();
}

bool SummarizationStorage::Get(const KeyType& key, Summarization& result)
{
    return summarization_db_.get(key, result);
}

bool SummarizationStorage::IsRebuildSummarizeRequired(
        const KeyType& key,
        const Summarization& value)
{
    Summarization summarization;
    return !summarization_db_.get(key, summarization) || summarization != value;
}

}

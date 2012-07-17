#include "CustomRankManager.h"
#include "CustomRankScorer.h"

namespace sf1r
{

CustomRankManager::CustomRankManager(const std::string& dbPath)
    : db_(dbPath)
{
}

void CustomRankManager::flush()
{
    db_.flush();
}

bool CustomRankManager::setDocIdList(
    const std::string& query,
    const DocIdList& docIdList
)
{
    return db_.update(query, docIdList);
}

bool CustomRankManager::getDocIdList(
    const std::string& query,
    DocIdList& docIdList
)
{
    docIdList.clear();

    return db_.get(query, docIdList);
}

CustomRankScorer* CustomRankManager::getScorer(const std::string& query)
{
    DocIdList docIdList;

    if (!db_.get(query, docIdList) || docIdList.empty())
        return NULL;

    return new CustomRankScorer(docIdList);
}

} // namespace sf1r

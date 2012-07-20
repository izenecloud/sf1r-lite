#include "CustomRankManager.h"
#include "CustomRankScorer.h"

namespace
{

class GetKeyFunc
{
public:
    GetKeyFunc(std::vector<std::string>& keys) : keys_(keys) {}

    void operator()(
        const std::string& key,
        const sf1r::CustomRankManager::DocIdList& value
    )
    {
        if (! value.empty())
        {
            keys_.push_back(key);
        }
    }

private:
    std::vector<std::string>& keys_;
};

}

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

bool CustomRankManager::getQueries(std::vector<std::string>& queries)
{
    GetKeyFunc func(queries);

    return db_.forEach(func);
}

} // namespace sf1r

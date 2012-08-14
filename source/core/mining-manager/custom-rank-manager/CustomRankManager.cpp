#include "CustomRankManager.h"
#include "CustomRankScorer.h"
#include "CustomDocIdConverter.h"
#include <document-manager/DocumentManager.h>

namespace
{

class GetKeyFunc
{
public:
    GetKeyFunc(std::vector<std::string>& keys) : keys_(keys) {}

    void operator()(
        const std::string& key,
        const sf1r::CustomRankDocStr& value
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

CustomRankManager::CustomRankManager(
    const std::string& dbPath,
    CustomDocIdConverter& docIdConverter,
    const DocumentManager* docManager
)
    : db_(dbPath)
    , docIdConverter_(docIdConverter)
    , docManager_(docManager)
{
}

void CustomRankManager::flush()
{
    db_.flush();
}

bool CustomRankManager::setCustomValue(
    const std::string& query,
    const CustomRankDocStr& customDocStr
)
{
    return db_.update(query, customDocStr);
}

bool CustomRankManager::getCustomValue(
    const std::string& query,
    CustomRankDocId& customDocId
)
{
    CustomRankDocStr customDocStr;

    return db_.get(query, customDocStr) &&
           docIdConverter_.convert(customDocStr, customDocId);
}

CustomRankScorer* CustomRankManager::getScorer(const std::string& query)
{
    CustomRankDocId customDocId;

    if (! getCustomValue(query, customDocId))
        return NULL;

    removeDeletedDocs_(customDocId.topIds);

    if (customDocId.empty())
        return NULL;

    return new CustomRankScorer(customDocId);
}

void CustomRankManager::removeDeletedDocs_(std::vector<docid_t>& docIds)
{
    if (! docManager_)
        return;

    const std::size_t size = docIds.size();
    std::size_t j = 0;

    for (std::size_t i = 0; i < size; ++i)
    {
        if (! docManager_->isDeleted(docIds[i]))
        {
            docIds[j] = docIds[i];
            ++j;
        }
    }

    docIds.resize(j);
}

bool CustomRankManager::getQueries(std::vector<std::string>& queries)
{
    GetKeyFunc func(queries);

    return db_.forEach(func);
}

} // namespace sf1r

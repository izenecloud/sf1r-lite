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
        const sf1r::CustomRankValue& value
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

bool CustomRankManager::setCustomValue(
    const std::string& query,
    const CustomRankValue& customValue
)
{
    return db_.update(query, customValue);
}

bool CustomRankManager::getCustomValue(
    const std::string& query,
    CustomRankValue& customValue
)
{
    customValue.clear();

    return db_.get(query, customValue);
}

CustomRankScorer* CustomRankManager::getScorer(const std::string& query)
{
    CustomRankValue customValue;

    if (!db_.get(query, customValue) || customValue.empty())
        return NULL;

    return new CustomRankScorer(customValue);
}

bool CustomRankManager::getQueries(std::vector<std::string>& queries)
{
    GetKeyFunc func(queries);

    return db_.forEach(func);
}

} // namespace sf1r

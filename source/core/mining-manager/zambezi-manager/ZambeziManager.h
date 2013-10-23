/**
 * @file ZambeziManager.h
 * @brief search in the zambezi inverted index.
 */

#ifndef SF1R_ZAMBEZI_MANAGER_H
#define SF1R_ZAMBEZI_MANAGER_H

#include <common/inttypes.h>
#include <search-manager/NumericPropertyTableBuilder.h>
#include <ir/Zambezi/AttrScoreInvertedIndex.hpp>
#include <common/PropSharedLockSet.h>
#include <string>
#include <vector>

namespace sf1r
{

namespace faceted
{
class GroupManager;
class AttrManager;
}

class DocumentManager;
class MiningTask;
class ZambeziConfig;

class ZambeziManager
{
public:
    ZambeziManager(
            const ZambeziConfig& config,
            faceted::AttrManager* attrManager,
            NumericPropertyTableBuilder* numericTableBuilder);

    bool open();

    MiningTask* createMiningTask(DocumentManager& documentManager);

    void search(
        const std::vector<std::pair<std::string, int> >& tokens,
        const boost::function<bool(uint32_t)>& filter,
        uint32_t limit,
        std::vector<docid_t>& docids,
        std::vector<uint32_t>& scores);

    void NormalizeScore(
        std::vector<docid_t>& docids,
        std::vector<float>& scores,
        std::vector<float>& productScores,
        PropSharedLockSet &sharedLockSet);

private:
    const ZambeziConfig& config_;

    faceted::AttrManager* attrManager_;

    izenelib::ir::Zambezi::AttrScoreInvertedIndex indexer_; // vector : indexer_ list;

    NumericPropertyTableBuilder* numericTableBuilder_;
};

} // namespace sf1r

#endif // SF1R_ZAMBEZI_MANAGER_H

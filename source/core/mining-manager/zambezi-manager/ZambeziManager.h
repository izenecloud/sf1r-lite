/**
 * @file ZambeziManager.h
 * @brief search in the zambezi inverted index.
 */

#ifndef SF1R_ZAMBEZI_MANAGER_H
#define SF1R_ZAMBEZI_MANAGER_H

#include <common/inttypes.h>

#include <ir/Zambezi/AttrScoreInvertedIndex.hpp>
#include <glog/logging.h>
#include <util/ClockTimer.h>

#include <string>
#include <vector>

namespace sf1r
{

class DocumentManager;
class MiningTask;
class ZambeziConfig;

class ZambeziManager
{
public:
    ZambeziManager(const ZambeziConfig& config);

    bool open();

    MiningTask* createMiningTask(DocumentManager& documentManager);

    template <class FilterType>
    void search(
        const std::vector<std::pair<std::string, int> >& tokens,
        const FilterType& filter,
        uint32_t limit,
        std::vector<docid_t>& docids,
        std::vector<uint32_t>& scores)
    {
        izenelib::util::ClockTimer timer;

        indexer_.retrievalAndFiltering(tokens, filter, limit, false, docids, scores);

        LOG(INFO) << "zambezi returns docid num: " << docids.size()
                  << ", costs: " << timer.elapsed() << " seconds";
    }

private:
    const ZambeziConfig& config_;

    izenelib::ir::Zambezi::AttrScoreInvertedIndex indexer_;
};

} // namespace sf1r

#endif // SF1R_ZAMBEZI_MANAGER_H

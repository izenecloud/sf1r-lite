/**
 * @file ZambeziManager.h
 * @brief search in the zambezi inverted index.
 */

#ifndef SF1R_ZAMBEZI_MANAGER_H
#define SF1R_ZAMBEZI_MANAGER_H

#include <common/inttypes.h>
#include <ir/Zambezi/InvertedIndex.hpp>
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

    void search(
        const std::vector<std::string>& tokens,
        std::size_t limit,
        std::vector<docid_t>& docids,
        std::vector<float>& scores);

private:
    const ZambeziConfig& config_;

    izenelib::ir::Zambezi::InvertedIndex indexer_;
};

} // namespace sf1r

#endif // SF1R_ZAMBEZI_MANAGER_H

#include "ZambeziManager.h"
#include "ZambeziMiningTask.h"
#include <configuration-manager/ZambeziConfig.h>
#include <glog/logging.h>
#include <fstream>

using namespace sf1r;

namespace
{
const izenelib::ir::Zambezi::Algorithm kAlgorithm =
    izenelib::ir::Zambezi::BWAND_AND;
}

ZambeziManager::ZambeziManager(const ZambeziConfig& config)
    : config_(config)
{
}

bool ZambeziManager::open()
{
    std::ifstream ifs(config_.indexFilePath.c_str(), std::ios_base::binary);
    if (! ifs)
        return true;

    try
    {
        indexer_.load(ifs);
    }
    catch (const std::exception& e)
    {
        LOG(ERROR) << "exception in read file: " << e.what()
                   << ", path: " << config_.indexFilePath;
        return false;
    }

    return true;
}

MiningTask* ZambeziManager::createMiningTask(DocumentManager& documentManager)
{
    return new ZambeziMiningTask(config_, documentManager, indexer_);
}

void ZambeziManager::search(
    const std::vector<std::string>& tokens,
    std::size_t limit,
    std::vector<docid_t>& docids,
    std::vector<float>& scores)
{
    indexer_.retrieval(kAlgorithm, tokens, limit, docids, scores);
}

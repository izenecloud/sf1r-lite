#include "ZambeziManager.h"
#include "ZambeziMiningTask.h"
#include <configuration-manager/ZambeziConfig.h>
#include <util/ClockTimer.h>
#include <glog/logging.h>
#include <fstream>

using namespace sf1r;

namespace
{
const bool kIsReverseIndex = true;

const izenelib::ir::Zambezi::Algorithm kAlgorithm =
    izenelib::ir::Zambezi::SVS;
}

ZambeziManager::ZambeziManager(const ZambeziConfig& config)
    : config_(config)
    , indexer_(kIsReverseIndex)
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
    izenelib::util::ClockTimer timer;

    std::vector<uint32_t> intScores;
    indexer_.retrieval(kAlgorithm, tokens, limit, docids, intScores);

    scores.assign(intScores.begin(), intScores.end());

    LOG(INFO) << "zambezi returns docid num: " << docids.size()
              << ", costs :" << timer.elapsed() << " seconds"
              << std::endl;
}

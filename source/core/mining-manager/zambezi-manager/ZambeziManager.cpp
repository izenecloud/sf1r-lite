#include "ZambeziManager.h"
#include "ZambeziMiningTask.h"

#include <configuration-manager/ZambeziConfig.h>
#include <fstream>

namespace sf1r
{

ZambeziManager::ZambeziManager(const ZambeziConfig& config)
    : config_(config)
    , indexer_(config_.poolSize, config_.poolCount, config_.reverse)
{
}

bool ZambeziManager::open()
{
    const std::string& path = config_.indexFilePath;
    std::ifstream ifs(path.c_str(), std::ios_base::binary);

    if (! ifs)
        return true;

    LOG(INFO) << "loading zambezi index path: " << path;

    try
    {
        indexer_.load(ifs);
    }
    catch (const std::exception& e)
    {
        LOG(ERROR) << "exception in read file: " << e.what()
                   << ", path: " << path;
        return false;
    }

    LOG(INFO) << "finished loading zambezi index, total doc num: "
              << indexer_.totalDocNum();

    return true;
}

MiningTask* ZambeziManager::createMiningTask(DocumentManager& documentManager)
{
    return new ZambeziMiningTask(config_, documentManager, indexer_);
}

}

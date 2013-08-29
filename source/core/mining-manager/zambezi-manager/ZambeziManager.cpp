#include "ZambeziManager.h"
#include "ZambeziMiningTask.h"
#include <glog/logging.h>
#include <fstream>

using namespace sf1r;

namespace
{
const izenelib::ir::Zambezi::Algorithm kAlgorithm =
    izenelib::ir::Zambezi::BWAND_AND;
}

ZambeziManager::ZambeziManager(
    const std::string& indexPropName,
    const std::string& indexFilePath)
    : indexPropName_(indexPropName)
    , indexFilePath_(indexFilePath)
{
}

bool ZambeziManager::open()
{
    std::ifstream ifs(indexFilePath_.c_str(), std::ios_base::binary);
    if (! ifs)
        return true;

    try
    {
        indexer_.load(ifs);
    }
    catch (const std::exception& e)
    {
        LOG(ERROR) << "exception in read file: " << e.what()
                   << ", path: " << indexFilePath_;
        return false;
    }

    return true;
}

ZambeziMiningTask* ZambeziManager::createMiningTask(DocumentManager& documentManager)
{
    return new ZambeziMiningTask(documentManager,
                                 indexer_,
                                 indexPropName_,
                                 indexFilePath_);
}

void ZambeziManager::search(
    const std::vector<std::string>& tokens,
    std::size_t limit,
    std::vector<docid_t>& docids,
    std::vector<float>& scores)
{
    indexer_.retrieval(kAlgorithm, tokens, limit, docids, scores);
}

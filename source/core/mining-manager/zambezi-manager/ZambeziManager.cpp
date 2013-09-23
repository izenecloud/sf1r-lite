#include "ZambeziManager.h"
#include "ZambeziMiningTask.h"

#include "../group-manager/PropValueTable.h" // pvid_t
#include "../group-manager/GroupManager.h"
#include "../attr-manager/AttrManager.h"
#include "../attr-manager/AttrTable.h"

#include <common/PropSharedLock.h>
#include <configuration-manager/ZambeziConfig.h>
#include <util/ClockTimer.h>
#include <glog/logging.h>
#include <fstream>
#include <math.h>

using namespace sf1r;

namespace
{
const izenelib::ir::Zambezi::Algorithm kAlgorithm =
    izenelib::ir::Zambezi::SVS;
}

ZambeziManager::ZambeziManager(const ZambeziConfig& config, faceted::AttrManager* attrManager)
    : config_(config)
    , attrManager_(attrManager)
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
    
    //AttrTable::ScopedReadLock lock(AttrTable::getMutex());

    for (unsigned int i = 0; i < docids.size(); ++i)
    {
        faceted::AttrTable::ValueIdList attrvids;
        attrManager_->getAttrTable().getValueIdList(docids[i], attrvids);
        uint32_t attr_size = attrvids.size();
        if (i < 100)
        {
            std::cout << "    intScores[" << i << "]:" << intScores[i];
        }
        std::cout << std::endl;
        float score = intScores[i]*pow(attr_size, 0.3);

        // normalize score to <0,1>;

        scores.push_back(score);
    }

    LOG(INFO) << "zambezi returns docid num: " << docids.size()
              << ", costs :" << timer.elapsed() << " seconds"
              << std::endl;
}

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
#include <algorithm>

using namespace sf1r;

namespace
{
const bool kIsReverseIndex = true;

const izenelib::ir::Zambezi::Algorithm kAlgorithm =
    izenelib::ir::Zambezi::SVS;
}

ZambeziManager::ZambeziManager(const ZambeziConfig& config, faceted::AttrManager* attrManager)
    : config_(config)
    , attrManager_(attrManager)
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
    const boost::function<bool(uint32_t)>& filter,
    std::size_t limit,
    std::vector<docid_t>& docids,
    std::vector<float>& scores)
{
    izenelib::util::ClockTimer timer;

    std::vector<uint32_t> intScores;
    indexer_.retrievalAndFiltering(kAlgorithm, tokens, filter, limit, docids, intScores);
    faceted::AttrTable* attTable = NULL;

    if (attrManager_)
    {
        attTable = &(attrManager_->getAttrTable());
        attTable->lockShared();
    }
    float maxScore = 1;
    uint32_t attr_size = 1;
    for (uint32_t i = 0; i < docids.size(); ++i)
    {
        if (attTable)
        {
            faceted::AttrTable::ValueIdList attrvids;
            attTable->getValueIdList(docids[i], attrvids);
            attr_size = std::min(attrvids.size(), size_t(20));
        }

        float score = intScores[i]*pow(attr_size, 0.3);
        if (score > maxScore)
            maxScore = score;

        scores.push_back(score);
    }

    if (attTable)
        attTable->unlockShared();


    for (uint32_t i = 0; i < scores.size(); ++i)
    {
        scores[i] /= maxScore;
    }

    LOG(INFO) << "zambezi returns docid num: " << docids.size()
              << ", costs :" << timer.elapsed() << " seconds";
}

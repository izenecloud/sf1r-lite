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
const izenelib::ir::Zambezi::Algorithm kAlgorithm =
    izenelib::ir::Zambezi::SVS;
}

ZambeziManager::ZambeziManager(
        const ZambeziConfig& config,
        faceted::AttrManager* attrManager,
        NumericPropertyTableBuilder* numericTableBuilder)
    : config_(config)
    , attrManager_(attrManager)
    , indexer_(config_.poolSize, config_.poolCount, config_.reverse)
    , numericTableBuilder_(numericTableBuilder)
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

void ZambeziManager::search(
    const std::vector<std::pair<std::string, int> >& tokens,
    const boost::function<bool(uint32_t)>& filter,
    uint32_t limit,
    std::vector<docid_t>& docids,
    std::vector<uint32_t>& scores)
{
    izenelib::util::ClockTimer timer;

    indexer_.retrievalAndFiltering(kAlgorithm, tokens, filter, limit, false, docids, scores);

    LOG(INFO) << "zambezi returns docid num: " << docids.size()
              << ", costs :" << timer.elapsed() << " seconds";
}

void ZambeziManager::NormalizeScore(
    std::vector<docid_t>& docids,
    std::vector<float>& scores,
    std::vector<float>& productScores,
    PropSharedLockSet &sharedLockSet)
{
    faceted::AttrTable* attTable = NULL;

    if (attrManager_)
    {
        attTable = &(attrManager_->getAttrTable());
        sharedLockSet.insertSharedLock(attTable);
    }
    float maxScore = 1;

    std::string propName = "itemcount";
    std::string propName_comment = "CommentCount";

    boost::shared_ptr<NumericPropertyTableBase> numericTable =
        numericTableBuilder_->createPropertyTable(propName);

    boost::shared_ptr<NumericPropertyTableBase> numericTable_comment =
        numericTableBuilder_->createPropertyTable(propName_comment);

    if (numericTable)
        sharedLockSet.insertSharedLock(numericTable.get());

    if (numericTable_comment)
        sharedLockSet.insertSharedLock(numericTable_comment.get());

    for (uint32_t i = 0; i < docids.size(); ++i)
    {
        uint32_t attr_size = 1;
        if (attTable)
        {
            faceted::AttrTable::ValueIdList attrvids;
            attTable->getValueIdList(docids[i], attrvids);
            attr_size = std::min(attrvids.size(), size_t(10));
        }

        int32_t itemcount = 1;
        if (numericTable)
        {
            numericTable->getInt32Value(docids[i], itemcount, false);
            attr_size += std::min(itemcount, 50);
        }

        if (numericTable_comment)
        {
            int32_t commentcount = 1;
            numericTable_comment->getInt32Value(docids[i], commentcount, false);
            attr_size += pow(std::min((float)commentcount, 500.), 0.7);
        }

        scores[i] = scores[i] * pow(attr_size, 0.5);
        if (scores[i] > maxScore)
            maxScore = scores[i];
    }

    for (unsigned int i = 0; i < scores.size(); ++i)
    {
        scores[i] = int(scores[i] / maxScore * 100) + productScores[i];
    }
}

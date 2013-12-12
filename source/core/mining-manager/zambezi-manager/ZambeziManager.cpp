#include "ZambeziManager.h"
#include "ZambeziMiningTask.h"

#include "../group-manager/PropValueTable.h" // pvid_t
#include "../group-manager/GroupManager.h"
#include "../attr-manager/AttrManager.h"
#include "../attr-manager/AttrTable.h"
#include "util/fmath/fmath.hpp"

#include <configuration-manager/ZambeziConfig.h>
#include <fstream>
#include <math.h>
#include <algorithm>

namespace sf1r
{

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
    std::string propName_sales = "SalesAmount";

    boost::shared_ptr<NumericPropertyTableBase> numericTable =
        numericTableBuilder_->createPropertyTable(propName);

    boost::shared_ptr<NumericPropertyTableBase> numericTable_comment =
        numericTableBuilder_->createPropertyTable(propName_comment);

    boost::shared_ptr<NumericPropertyTableBase> numericTable_sales =
        numericTableBuilder_->createPropertyTable(propName_sales);

    if (numericTable)
        sharedLockSet.insertSharedLock(numericTable.get());

    if (numericTable_comment)
        sharedLockSet.insertSharedLock(numericTable_comment.get());

    for (uint32_t i = 0; i < docids.size(); ++i)
    {
        int32_t itemcount = 1;
        if (numericTable)
            numericTable->getInt32Value(docids[i], itemcount, false);

        uint32_t attr_size = 1;
        if (attTable)
        {
            faceted::AttrTable::ValueIdList attrvids;
            attTable->getValueIdList(docids[i], attrvids);
            attr_size += std::min(attrvids.size(), size_t(30))*10.;
        }
/*
        if (numericTable_comment)
        {
            int32_t commentcount = 0;
            numericTable_comment->getInt32Value(docids[i], commentcount, false);
            attr_size += (double)commentcount/itemcount;
        }

        if (numericTable_sales)
        {
            int32_t salescount = 0;
            numericTable_sales->getInt32Value(docids[i], salescount, false);
            attr_size += (double)salescount/itemcount;
        }
*/
        scores[i] = scores[i] + attr_size;
        if (scores[i] > maxScore)
            maxScore = scores[i];
    }

    for (unsigned int i = 0; i < scores.size(); ++i)
    {
        float x = fmath::exp((float)(scores[i]/60000.*-1));
        x = (1-x)/(1+x);
        scores[i] = int((x*100. + productScores[i])/10+0.5)*10;
    }
}

}

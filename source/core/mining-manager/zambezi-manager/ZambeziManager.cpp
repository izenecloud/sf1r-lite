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
    void init();
}

bool ZambeziManager::open_1()
{
    const std::string& basePath = config_.indexFilePath;

    for (std::vector<std::string>::iterator i = propertyList_.begin(); i != propertyList_.end(); ++i)
    {
        std::string path = basePath + "_" + *i; // index.bin_Title
        std::ifstream ifs(path.c_str(), std::ios_base::binary);

        if (! ifs)
            return false;// orignal return true;

        LOG(INFO) << "loading zambezi index for propery: " << *i << ", path" << path;

        try
        {
            property_index_map_[*i].load(ifs);
        }
        catch (const std::exception& e)
        {
            LOG(ERROR) << "exception in read file: " << e.what()
                   << ", path: " << path;
            return false;
        }
    }

    LOG(INFO) << "Finished loading zambezi index";

    return true;
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
    const std::vector<std::string>& propertyList,
    std::vector<docid_t>& docids,
    std::vector<uint32_t>& scores)
{
    izenelib::util::ClockTimer timer;
    if (propertyList.size() == 1)
    {    
        property_index_map_[propertyList[0]].retrievalAndFiltering(kAlgorithm, tokens, filter, limit, docids, scores);// if need to use filter;
        LOG(INFO) << "zambezi returns docid num: " << docids.size()
                  << ", costs :" << timer.elapsed() << " seconds";
        return;
    }

    std::vector<std::vector<docid_t> > docidsList;
    docidsList.resize(propertyList.size());
    std::vector<std::vector<uint32_t> > scoresList;
    scoresList.resize(propertyList.size());

    for (unsigned int i = 0; i < propertyList.size(); ++i)
    {
        property_index_map_[propertyList[i]].retrievalAndFiltering(kAlgorithm, tokens, filter, limit, docidsList[i], scoresList[i]); // add new interface;
    }

    merge_(docidsList, scoresList, docids, scores);

    LOG(INFO) << "zambezi returns docid num: " << docids.size()
              << ", costs :" << timer.elapsed() << " seconds";
}

void ZambeziManager::merge_(
        const std::vector<std::vector<docid_t> >& docidsList,
        const std::vector<std::vector<uint32_t> >& scoresList,
        std::vector<docid_t>& docids,
        std::vector<uint32_t>& scores)
{

}

void ZambeziManager::search(
    const std::vector<std::pair<std::string, int> >& tokens,
    const boost::function<bool(uint32_t)>& filter,
    uint32_t limit,
    std::vector<docid_t>& docids,
    std::vector<uint32_t>& scores)
{
    izenelib::util::ClockTimer timer;

    indexer_.retrievalAndFiltering(kAlgorithm, tokens, filter, limit, docids, scores);

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
            if (itemcount != 0)
                attr_size += std::min(commentcount/itemcount, 100);
            else
                attr_size += std::min(commentcount, 100);

        }

        scores[i] = scores[i] * pow(attr_size, 0.3);
        if (scores[i] > maxScore)
            maxScore = scores[i];
    }

    for (unsigned int i = 0; i < scores.size(); ++i)
    {
        scores[i] = int(scores[i] / maxScore * 100) + productScores[i];
    }
}

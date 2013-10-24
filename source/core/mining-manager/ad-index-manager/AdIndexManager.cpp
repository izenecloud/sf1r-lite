/*
 *  AdIndexManager.cpp
 */

#include "AdIndexManager.h"
#include <search-manager/HitQueue.h>
#include <algorithm>

const size_t MAX_AD_COUNT = 20;

namespace sf1r
{

AdIndexManager::AdIndexManager(
        const std::string& path,
        boost::shared_ptr<DocumentManager>& dm,
        NumericPropertyTableBuilder* ntb)
    :indexPath_(path), documentManager_(dm), numericTableBuilder_(ntb)
{
}

AdIndexManager::~AdIndexManager()
{
    if(adMiningTask_)
        delete adMiningTask_;
}

bool AdIndexManager::buildMiningTask()
{
    adMiningTask_ = new AdMiningTask(indexPath_, documentManager_);

    adMiningTask_->load();
    return true;
}

bool AdIndexManager::search(const std::vector<std::pair<std::string, std::string> >& info,
        std::vector<docid_t>& docids,
        std::vector<float>& topKScoreRankList,
        std::size_t& totalCount)
{
    boost::unordered_set<uint32_t> dnfIDs;

    adMiningTask_->retrieve(info, dnfIDs);

    std::string propName = "Price";
    std::string propName_mode = "BidMode";
    PropSharedLockSet propSharedLockSet;

    boost::shared_ptr<NumericPropertyTableBase> numericTable =
        numericTableBuilder_->createPropertyTable(propName);

    boost::shared_ptr<NumericPropertyTableBase> numericTable_mode =
        numericTableBuilder_->createPropertyTable(propName_mode);

    if ( numericTable )
        propSharedLockSet.insertSharedLock(numericTable.get());

    if ( numericTable_mode )
        propSharedLockSet.insertSharedLock(numericTable_mode.get());

    boost::shared_ptr<HitQueue> scoreItemQueue;

    uint32_t heapSize = std::min(MAX_AD_COUNT, dnfIDs.size());

    scoreItemQueue.reset(new ScoreSortedHitQueue(heapSize));

    LOG(INFO)<<"dnfIDs.size(): "<<dnfIDs.size()<<endl;
    for(boost::unordered_set<uint32_t>::iterator it = dnfIDs.begin();
            it != dnfIDs.end(); it++ )
    {
        if(!documentManager_->isDeleted(*it))
        {
            float price = 0.0;
            float score = 0.0;
            int32_t mode = 0;
            if(numericTable)
            {
                numericTable->getFloatValue(*it, price, false);
            }
            if(numericTable_mode)
            {
                numericTable_mode->getInt32Value(*it, mode, false);
            }
            if(mode == 0)
            {
                score = price;
            }
            else if(mode == 1)
            {
                //TODO
                // calculate CTR
                // calcutate eCPM
                score = price;
            }
            ScoreDoc scoreItem(*it, score);
            scoreItemQueue->insert(scoreItem);
        }
    }

    std::vector<docid_t> topDocids;
    std::vector<float> scores;

    unsigned int scoreSize = scoreItemQueue->size();
    for(unsigned int i=0; i<scoreSize;i++)
    {
        const ScoreDoc& scoreItem = scoreItemQueue->pop();
        topDocids.push_back(scoreItem.docId);
        scores.push_back(scoreItem.score);
    }

    std::vector<docid_t>::iterator it1 = topDocids.end();
    std::vector<float>::iterator it2 = scores.end();
    while(it1 != topDocids.begin())
    {
        it1--;it2--;
        docids.push_back(*it1);
        topKScoreRankList.push_back(*it2);
    }
    totalCount = docids.size();

    return true;
}

} //namespace sf1r

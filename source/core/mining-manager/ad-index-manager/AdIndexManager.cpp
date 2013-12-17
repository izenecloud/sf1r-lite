/*
 *  AdIndexManager.cpp
 */

#include "AdIndexManager.h"
#include "AdStreamSubscriber.h"
#include "AdSelector.h"
#include "AdClickPredictor.h"
#include <search-manager/HitQueue.h>
#include <algorithm>

namespace sf1r
{

static const int MAX_AD_COUNT = 20;
static const std::string adlog_topic = "b5manlog";

AdIndexManager::AdIndexManager(
        const std::string& indexPath,
        const std::string& clickPredictorWorkingPath,
        boost::shared_ptr<DocumentManager>& dm,
        NumericPropertyTableBuilder* ntb)
    : indexPath_(indexPath),
      clickPredictorWorkingPath_(clickPredictorWorkingPath),
      documentManager_(dm),
      numericTableBuilder_(ntb)
{
}

AdIndexManager::~AdIndexManager()
{
    // unsubscribe should make sure all callback finished and 
    // no any callback will be send later.
    AdStreamSubscriber::get()->unsubscribe(adlog_topic);

    if(adMiningTask_)
        delete adMiningTask_;

    ad_click_predictor_->stop();
    AdClickPredictor::get()->stop();
}

bool AdIndexManager::buildMiningTask()
{
    adMiningTask_ = new AdMiningTask(indexPath_, documentManager_);
    adMiningTask_->load();

    ad_click_predictor_ = AdClickPredictor::get();
    ad_click_predictor_->init(clickPredictorWorkingPath_);
    AdSelector::get()->init(ad_selector_data_path_, ad_click_predictor_);
    bool ret = AdStreamSubscriber::get()->subscribe(adlog_topic, boost::bind(&AdIndexManager::onAdStreamMessage, this, _1));
    if (!ret)
    {
        LOG(ERROR) << "subscribe the click log failed !!!!!!";
    }

    return true;
}

void AdIndexManager::onAdStreamMessage(const std::vector<AdMessage>& msg_list)
{
    static int cnt = 0;
    if (cnt % 10000 == 0)
    {
        for (size_t i = 0; i < msg_list.size(); ++i)
        {
            LOG(INFO) << "stream data: " << msg_list[i].body;
        }
        LOG(INFO) << "got ad stream data. size: " << msg_list.size() << ", total " << cnt;
    }
    cnt += msg_list.size();
    std::vector<std::pair<AdClickPredictor::AssignmentT, bool> > assignment_list;
    // read from stream msg and convert it to assignment list.
    for(size_t i = 0; i < assignment_list.size(); ++i)
    {
        ad_click_predictor_->update(assignment_list[i].first, assignment_list[i].second);
        if (assignment_list[i].second)
        {
            // get docid from adid.
            docid_t docid;
            AdSelector::get()->updateClicked(docid);
        }
        AdSelector::get()->updateSegments(assignment_list[i].first);
    }
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

    uint32_t heapSize = std::min((std::size_t)MAX_AD_COUNT, dnfIDs.size());

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
                // calculate CTR
                //double ctr = adClickPredictor_->predict(info);
                // calculate eCPM
                //score = ctr * price * 1000;
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

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

static const int MAX_SEARCH_AD_COUNT = 2000;
static const int MAX_SELECT_AD_COUNT = 20;
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
    AdSelector::get()->init(ad_selector_data_path_, ad_click_predictor_, documentManager_.get());
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
        AdSelector::get()->updateSegments(assignment_list[i].first, AdSelector::UserSeg);
    }
}

bool AdIndexManager::search(const std::vector<std::pair<std::string, std::string> >& info,
    std::vector<docid_t>& docids,
    std::vector<float>& topKScoreRankList,
    std::size_t& totalCount)
{
    std::string ad_search_query;

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

    // Note: ad feature can be attribute.
    std::vector<docid_t>  cpc_ads_result;
    std::vector<AdSelector::FeatureMapT> cpc_ads_features;
    cpc_ads_result.reserve(dnfIDs.size());
    cpc_ads_features.reserve(dnfIDs.size());

    uint32_t heapSize = std::min((std::size_t)MAX_SEARCH_AD_COUNT, dnfIDs.size());

    scoreItemQueue.reset(new ScoreSortedHitQueue(heapSize));

    LOG(INFO)<<"dnfIDs.size(): "<<dnfIDs.size()<<endl;
    std::vector<std::string> value_list;
    for(boost::unordered_set<uint32_t>::iterator it = dnfIDs.begin();
            it != dnfIDs.end(); it++ )
    {
        if(!documentManager_->isDeleted(*it))
        {
            float price = 0.0;
            float score = 0.0;
            int32_t mode = 0;
            if(numericTable_mode)
            {
                numericTable_mode->getInt32Value(*it, mode, false);
            }
            if(mode == 0)
            {
                if(numericTable)
                {
                    numericTable->getFloatValue(*it, price, false);
                }
                score = price;
            }
            else if(mode == 1)
            {
                cpc_ads_result.push_back(*it);
                cpc_ads_features.push_back(AdSelector::FeatureMapT());
                AdSelector::get()->getDefaultFeatures(cpc_ads_features.back(), AdSelector::AdSeg);
                for (AdSelector::FeatureMapT::iterator feature_it = cpc_ads_features.back().begin();
                    feature_it != cpc_ads_features.back().end(); ++feature_it)
                {
                    uint32_t featureid;
                    // for multivalue we can use a int value to stand for.
                    //featureTable[feature_it->first]->getFeatureId(*it, featureid, false);
                    //getValueListByFeatureId(featureid, value_list);
                    //
                    cpc_ads_features.back()[feature_it->first].swap(value_list);
                }
                continue;
            }
            ScoreDoc scoreItem(*it, score);
            scoreItemQueue->insert(scoreItem);
        }
    }
    LOG(INFO) << "begin select ads from cpc cand result : " << cpc_ads_result.size();
    // select some ads using some strategy to maximize the CPC.
    AdSelector::get()->select(info, cpc_ads_features, MAX_SELECT_AD_COUNT,
        cpc_ads_result, scored_result, MAX_SEARCH_AD_COUNT);
    LOG(INFO) << "end select ads from result.";
    // calculate eCPM
    for (std::size_t i = 0; i < cpc_ads_result.size(); ++i)
    {
        float price = 0;
        if(numericTable)
        {
            numericTable->getFloatValue(cpc_ads_result[i], price, false);
        }
        double score = scored_result[i] * price * 1000;
        ScoreDoc item(cpc_ads_result[i], score);
        scoreItemQueue->insert(item);
    }

    unsigned int scoreSize = scoreItemQueue->size();

    docids.resize(scoreSize);
    topKRankScoreList.resize(scoreSize);

    for (int i = scoreSize - 1; i >= 0; --i)
    {
        const ScoreDoc& item = scoreItemQueue->pop();
        docids[i] = item.docId;
        topKScoreRankList[i] = item.score;
    }
    totalCount = docids.size();

    return true;
}

} //namespace sf1r

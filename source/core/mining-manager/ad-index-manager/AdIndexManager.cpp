/*
 *  AdIndexManager.cpp
 */

#include "AdIndexManager.h"
#include "AdStreamSubscriber.h"
#include "AdSelector.h"
#include "AdClickPredictor.h"
#include <common/ResultType.h>
#include <mining-manager/group-manager/GroupManager.h>
#include <query-manager/ActionItem.h>
#include <search-manager/SearchBase.h>
#include <search-manager/HitQueue.h>
#include <algorithm>

namespace sf1r
{

static const int MAX_SEARCH_AD_COUNT = 20000;
static const int MAX_SELECT_AD_COUNT = 20;
static const std::string adlog_topic = "b5manlog";

AdIndexManager::AdIndexManager(
        const std::string& indexPath,
        const std::string& clickPredictorWorkingPath,
        const std::string& ad_resource_path,
        boost::shared_ptr<DocumentManager>& dm,
        NumericPropertyTableBuilder* ntb,
        SearchBase* searcher,
        faceted::GroupManager* grp_mgr)
    : indexPath_(indexPath),
      clickPredictorWorkingPath_(clickPredictorWorkingPath),
      ad_selector_data_path_(ad_resource_path + "/ad_selector"),
      documentManager_(dm),
      numericTableBuilder_(ntb),
      ad_searcher_(searcher),
      groupManager_(grp_mgr)
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

inline std::string AdIndexManager::getValueStrFromPropId(uint32_t pvid)
{
    return boost::lexical_cast<std::string>(pvid) + "-";
    //    // may have multi value.
    //    std::vector<std::string> value;
    //    pvt_list[feature_index]->propValuePath(propids[k], value);
    //    if (value.empty()) continue;
    //    value_list.push_back(value[0]);
}

void AdIndexManager::postMining(docid_t startid, docid_t endid)
{
    PropSharedLockSet propSharedLockSet;
    std::vector<faceted::PropValueTable*> pvt_list;
    std::vector<std::set<std::string> > all_kinds_ad_segments;
    AdSelector::FeatureMapT def_feature_names;
    AdSelector::get()->getDefaultFeatures(def_feature_names, AdSelector::AdSeg);
    for (AdSelector::FeatureMapT::const_iterator feature_it = def_feature_names.begin();
        feature_it != def_feature_names.end(); ++feature_it)
    {
        faceted::PropValueTable* pvt = groupManager_->getPropValueTable(feature_it->first);
        if (pvt)
            propSharedLockSet.insertSharedLock(pvt);
        pvt_list.push_back(pvt);
    }
    all_kinds_ad_segments.resize(pvt_list.size());

    faceted::PropValueTable::PropIdList propids;
    for (docid_t i = startid; i <= endid; ++i)
    {
        for (std::size_t feature_index = 0; feature_index < pvt_list.size(); ++feature_index)
        {
            if (pvt_list[feature_index])
            {
                pvt_list[feature_index]->getPropIdList(i, propids);
                // get the string value for the id.
                for (size_t k = 0; k < propids.size(); ++k)
                {
                    all_kinds_ad_segments[feature_index].insert(getValueStrFromPropId(propids[k]));
                }
            }
        }
    }

    AdSelector::FeatureMapT::const_iterator it = def_feature_names.begin();
    for (std::size_t i = 0; i < all_kinds_ad_segments.size(); ++i)
    {
        LOG(INFO) << "found total segments : " << all_kinds_ad_segments[i].size() << "  for : " << it->first;
        AdSelector::get()->updateSegments(it->first, all_kinds_ad_segments[i], AdSelector::AdSeg);
    }
}

void AdIndexManager::rankAndSelect(const std::vector<std::pair<std::string, std::string> > userinfo,
    std::vector<docid_t>& docids,
    std::vector<float>& topKRankScoreList,
    std::size_t& totalCount)
{
    LOG(INFO) << "begin rank ads from searched result : " << docids.size();
    typedef std::vector<docid_t>::iterator InputIterator;

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
    std::vector<docid_t>  cpc_ads_result;
    std::vector<AdSelector::FeatureMapT> cpc_ads_features;
    cpc_ads_result.reserve(docids.size());
    cpc_ads_features.reserve(docids.size());

    uint32_t heapSize = std::min((std::size_t)MAX_SEARCH_AD_COUNT, docids.size());
    scoreItemQueue.reset(new ScoreSortedHitQueue(heapSize));

    std::vector<faceted::PropValueTable*> pvt_list;
    AdSelector::FeatureMapT def_feature_names;
    AdSelector::get()->getDefaultFeatures(def_feature_names, AdSelector::AdSeg);
    for (AdSelector::FeatureMapT::iterator feature_it = def_feature_names.begin();
        feature_it != def_feature_names.end(); ++feature_it)
    {
        faceted::PropValueTable* pvt = groupManager_->getPropValueTable(feature_it->first);
        if (pvt)
            propSharedLockSet.insertSharedLock(pvt);
        pvt_list.push_back(pvt);
    }

    std::vector<std::string> value_list;
    for(InputIterator it = docids.begin();
            it != docids.end(); it++ )
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
                faceted::PropValueTable::PropIdList propids;
                std::vector<std::string> str_values;

                std::size_t feature_index = 0;
                for (AdSelector::FeatureMapT::iterator feature_it = cpc_ads_features.back().begin();
                    feature_it != cpc_ads_features.back().end(); ++feature_it)
                {
                    if (pvt_list[feature_index])
                    {
                        pvt_list[feature_index]->getPropIdList(*it, propids);
                        // get the string value for the id.
                        for (size_t k = 0; k < propids.size(); ++k)
                        {
                            str_values.push_back(getValueStrFromPropId(propids[k]));
                        }
                    }
                    feature_it->second.swap(str_values);
                    ++feature_index;
                }
                continue;
            }
            ScoreDoc scoreItem(*it, score);
            scoreItemQueue->insert(scoreItem);
        }
    }
    LOG(INFO) << "begin select ads from cpc cand result : " << cpc_ads_result.size();
    // select some ads using some strategy to maximize the CPC.
    std::vector<double> score_list;
    AdSelector::get()->select(userinfo, cpc_ads_features, MAX_SELECT_AD_COUNT,
        cpc_ads_result, score_list, MAX_SEARCH_AD_COUNT);
    LOG(INFO) << "end select ads from result.";
    // calculate eCPM
    for (std::size_t i = 0; i < cpc_ads_result.size(); ++i)
    {
        float price = 0;
        if(numericTable)
        {
            numericTable->getFloatValue(cpc_ads_result[i], price, false);
        }
        double score = score_list[i] * price * 1000;
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
        topKRankScoreList[i] = item.score;
    }
    totalCount = docids.size();
}

bool AdIndexManager::searchByQuery(const SearchKeywordOperation& actionOperation,
    KeywordSearchResult& searchResult)
{
    if (!ad_searcher_)
        return false;
    // using the DNF as Attributes.
    bool ret = ad_searcher_->search(actionOperation, searchResult, MAX_SEARCH_AD_COUNT, 0);
    const std::vector<std::pair<std::string, std::string> > userinfo;
    if (ret)
    {
        rankAndSelect(userinfo, searchResult.topKDocs_, searchResult.topKRankScoreList_, searchResult.totalCount_);
    }
    return ret;
}

bool AdIndexManager::searchByDNF(const std::vector<std::pair<std::string, std::string> >& info,
    std::vector<docid_t>& docids,
    std::vector<float>& topKRankScoreList,
    std::size_t& totalCount)
{
    boost::unordered_set<uint32_t> dnfIDs;
    adMiningTask_->retrieve(info, dnfIDs);
    LOG(INFO)<< "dnfIDs.size(): "<< dnfIDs.size() << std::endl;

    docids.resize(dnfIDs.size());

    std::size_t i = 0;
    for(boost::unordered_set<uint32_t>::iterator it = dnfIDs.begin();
            it != dnfIDs.end(); it++ )
    {
        docids[i++] = *it;
    }

    rankAndSelect(info, docids, topKRankScoreList, totalCount);
    return true;
}

} //namespace sf1r

#include "AggregatorManager.h"

#include <common/ResultType.h>
#include <mining-manager/MiningManager.h>
#include <mining-manager/taxonomy-generation-submanager/TaxonomyRep.h>
#include <mining-manager/taxonomy-generation-submanager/TaxonomyGenerationSubManager.h>
namespace sf1r
{

//int TOP_K_NUM = 1000;

void AggregatorManager::mergeSearchResult(KeywordSearchResult& result, const std::vector<std::pair<workerid_t, KeywordSearchResult> >& resultList)
{
    cout << "#[AggregatorManager::mergeSearchResult] " << resultList.size() << endl;

    size_t workerNum = resultList.size();

    // only one result
    if (workerNum == 1)
    {
        result = resultList[0].second;
        ///result.workerIdList_.resize(result.topKDocs_.size(), resultList[0].first);
        return;
    }

    cout << "resultPage(original) start: " << result.start_ << ", count: " << result.count_ << endl;

    // get basic info
    size_t overallTotalCount = 0;
    size_t overallResultCount = 0;

    for (size_t i = 0; i < workerNum; i++)
    {
        const KeywordSearchResult& wResult = resultList[i].second;
        overallTotalCount += wResult.totalCount_;
        overallResultCount += wResult.topKDocs_.size();
    }
    cout << "overallTotalCount: " << overallTotalCount << ",  overallResultCount: " << overallResultCount<<endl;

    // get page count info
    size_t resultCount = result.start_ + result.count_; // xxx
    if (result.start_ > overallResultCount)
    {
        result.start_ = overallResultCount;
    }
    if (resultCount > overallResultCount)
    {
        result.count_ = overallResultCount - result.start_;
    }
    cout << "resultPage      start: " << result.start_ << ", count: " << result.count_ << endl;

    result.topKDocs_.resize(resultCount);
    result.topKWorkerIds_.resize(resultCount);
    result.topKRankScoreList_.resize(resultCount);
    result.topKCustomRankScoreList_.resize(resultCount);

    // merge multiple results
    float max;
    size_t maxi;
    size_t* iter = new size_t[workerNum];
    memset(iter, 0, sizeof(size_t)*workerNum);
    for (size_t cnt = 0; cnt < resultCount; cnt++)
    {
        // find max score from head of multiple score lists (sorted).
        max = 0; maxi = size_t(-1);
        for (size_t i = 0; i < workerNum; i++)
        {
            const std::vector<float>& rankScoreList = resultList[i].second.topKRankScoreList_;
            if (iter[i] < rankScoreList.size() && rankScoreList[iter[i]] > max)
            {
                max = rankScoreList[iter[i]];
                maxi = i;
            }
        }

        cout << "max: " << max <<", maxi: "<< maxi<<", iter[maxi]: " << iter[maxi]<<endl;
        if (maxi == size_t(-1))
        {
            break;
        }

        // get a result
        const workerid_t& workerid = resultList[maxi].first;
        const KeywordSearchResult& wResult = resultList[maxi].second;

        result.topKDocs_[cnt] = wResult.topKDocs_[iter[maxi]];
        result.topKWorkerIds_[cnt] = workerid;
        result.topKRankScoreList_[cnt] = wResult.topKRankScoreList_[iter[maxi]];
        if (wResult.topKCustomRankScoreList_.size() > 0)
            result.topKCustomRankScoreList_[cnt] = wResult.topKCustomRankScoreList_[iter[maxi]];

        // next
        iter[maxi] ++;
    }

    delete[] iter;

}

void AggregatorManager::mergeSummaryResult(KeywordSearchResult& result, const std::vector<std::pair<workerid_t, KeywordSearchResult> >& resultList)
{
    cout << "#[AggregatorManager::mergeSummaryResult] " << resultList.size() << endl;

    size_t workerNum = resultList.size();

    // only one result
    if (workerNum == 1)
    {
        result = resultList[0].second;
        ///result.workerIdList_.resize(result.topKDocs_.size(), resultList[0].first);
        return;
    }
}

void AggregatorManager::splitResultByWorkerid(const KeywordSearchResult& result, std::map<workerid_t, boost::shared_ptr<KeywordSearchResult> >& resultMap)
{
    const std::vector<uint32_t>& topKWorkerIds = result.topKWorkerIds_;

    // split docs of current page by worker
    boost::shared_ptr<KeywordSearchResult> subResult;
    for (size_t i = result.start_; i < topKWorkerIds.size(); i ++)
    {
        if (i >= result.start_ + result.count_)
            break;

        workerid_t curWorkerid = topKWorkerIds[i];
        if (resultMap.find(curWorkerid) == resultMap.end())
        {
            subResult.reset(new KeywordSearchResult());
            // xxx, here just copy info needed for getting summary, mining result.
            subResult->propertyQueryTermList_ = result.propertyQueryTermList_;
            resultMap.insert(std::make_pair(curWorkerid, subResult));
        }
        else
        {
            subResult = resultMap[curWorkerid];
        }

        subResult->topKDocs_.push_back(result.topKDocs_[i]);
    }
}

////////////////////////////

void AggregatorManager::mergeSummaryResult(KeywordSearchResult& result, const std::vector<std::pair<workerid_t, boost::shared_ptr<KeywordSearchResult> > >& resultList)
{

}


void AggregatorManager::mergeMiningResult(KeywordSearchResult& result, const std::vector<std::pair<workerid_t, boost::shared_ptr<KeywordSearchResult> > >& resultList)
{
    if(!mining_manager_) return;
    boost::shared_ptr<TaxonomyGenerationSubManager> tg_manager = mining_manager_->GetTgManager();
    if(tg_manager)
    {
        std::vector<std::pair<uint32_t, idmlib::cc::CCInput32> > input_list;
        for(uint32_t i=0;i<resultList.size();i++)
        {
            const idmlib::cc::CCInput32& tg_input = resultList[i].second->tg_input;
            if(tg_input.concept_list.size()>0)
            {
                input_list.push_back(std::make_pair(resultList[i].first, tg_input) );
            }
        }
        if( input_list.size()>0)
        {
            idmlib::cc::CCInput64 input;
            tg_manager->AggregateInput(input_list, input);
            TaxonomyRep taxonomyRep;
            if( tg_manager->GetResult(input, taxonomyRep, result.neList_) )
            {
                taxonomyRep.fill(result);
            }
        }
    }
}

void AggregatorManager::SetMiningManager(const boost::shared_ptr<MiningManager>& mining_manager)
{
    mining_manager_ = mining_manager;
}

}

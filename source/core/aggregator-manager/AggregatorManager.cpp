#include "AggregatorManager.h"

#include <common/ResultType.h>
#include <mining-manager/MiningManager.h>
#include <mining-manager/taxonomy-generation-submanager/TaxonomyRep.h>
#include <mining-manager/taxonomy-generation-submanager/TaxonomyGenerationSubManager.h>
namespace sf1r
{

extern int TOP_K_NUM;

void AggregatorManager::mergeSearchResult(KeywordSearchResult& result, const std::vector<std::pair<workerid_t, KeywordSearchResult> >& resultList)
{
    cout << "#[AggregatorManager::mergeSearchResult] " << resultList.size() << endl;

    size_t workerNum = resultList.size();

    // only one result
    if (workerNum == 1)
    {
        result = resultList[0].second;
        result.topKWorkerIds_.resize(result.topKDocs_.size(), resultList[0].first);
        return;
    }

    // get basic info
    result.totalCount_ = 0;
    size_t overallResultCount = 0;

    for (size_t i = 0; i < workerNum; i++)
    {
        const KeywordSearchResult& wResult = resultList[i].second;
        //result.totalCount_ += ((wResult.totalCount_ > TOP_K_NUM) ? TOP_K_NUM : wResult.totalCount_);
        result.totalCount_ += wResult.totalCount_;
        overallResultCount += wResult.topKDocs_.size();
    }
    cout << "result.totalCount_: " << result.totalCount_ << ",  overallResultCount: " << overallResultCount<<endl;

    // get page count info
    if (result.start_ >= overallResultCount)
    {
        // page offset over flow
        return;
    }
    if (result.start_ + result.count_ > overallResultCount)
    {
        result.count_ = overallResultCount - result.start_;
    }
    size_t resultCount = overallResultCount < size_t(TOP_K_NUM) ? overallResultCount : TOP_K_NUM;

    //cout << "resultPage      start: " << result.start_ << ", count: " << result.count_ << endl;

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

        //cout << "max: " << max <<", maxi: "<< maxi<<", iter[maxi]: " << iter[maxi]<<endl;
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
        subResult->topKWorkerIds_.push_back(curWorkerid);
        subResult->topKPostionList_.push_back(i);
    }
}

////////////////////////////

void AggregatorManager::mergeSummaryResult(KeywordSearchResult& result, const std::vector<std::pair<workerid_t, boost::shared_ptr<KeywordSearchResult> > >& resultList)
{
    cout << "#[AggregatorManager::mergeSummaryResult] " << endl;

    size_t subResultNum = resultList.size();

    if (subResultNum <= 0)
        return;

    size_t topk = result.topKDocs_.size();
    size_t displayPropertyNum = resultList[0].second->snippetTextOfDocumentInPage_.size(); //xxx
    size_t isSummaryOn = resultList[0].second->rawTextOfSummaryInPage_.size(); //xxx

    // initialize summary info for result
    result.snippetTextOfDocumentInPage_.resize(displayPropertyNum);
    result.fullTextOfDocumentInPage_.resize(displayPropertyNum);
    if (isSummaryOn)
        result.rawTextOfSummaryInPage_.resize(displayPropertyNum);
    for (size_t dis = 0; dis < displayPropertyNum; dis++)
    {
        result.snippetTextOfDocumentInPage_[dis].resize(topk);
        result.fullTextOfDocumentInPage_[dis].resize(topk);
        if (isSummaryOn)
            result.rawTextOfSummaryInPage_[dis].resize(topk);
    }

    // merge
    //size_t curPos;
    size_t curSub;
    size_t* iter = new size_t[subResultNum];
    memset(iter, 0, sizeof(size_t)*subResultNum);

    for (size_t i = 0; i < topk; i++)
    {
        //curPos = 0;
        curSub = size_t(-1);
        for (size_t sub = 0; sub < subResultNum; sub++)
        {
            const std::vector<size_t>& subTopKPosList = resultList[sub].second->topKPostionList_;
            if (iter[sub] < subTopKPosList.size() && subTopKPosList[iter[sub]] == i)
            {
                //curPos = subTopKPosList[iter[sub]];
                curSub = sub;
                break;
            }
        }

        if (curSub == size_t(-1))
            continue;
        //cout << "curPos: " << i <<", curSub: "<< curSub<<", iter[curSub]: " << iter[curSub]<<endl;

        // get a result
        const boost::shared_ptr<KeywordSearchResult>& subResult = resultList[curSub].second;
        for (size_t dis = 0; dis < displayPropertyNum; dis++)
        {
            result.snippetTextOfDocumentInPage_[dis][i] = subResult->snippetTextOfDocumentInPage_[dis][iter[curSub]];
            result.fullTextOfDocumentInPage_[dis][i] = subResult->fullTextOfDocumentInPage_[dis][iter[curSub]];
            if (isSummaryOn)
                result.rawTextOfSummaryInPage_[dis][i] = subResult->rawTextOfSummaryInPage_[dis][iter[curSub]];
        }

        // next
        iter[curSub] ++;
    }

    delete[] iter;
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

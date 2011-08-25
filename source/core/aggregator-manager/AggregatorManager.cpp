#include "AggregatorManager.h"
#include "MergeComparator.h"

#include <common/ResultType.h>
#include <mining-manager/MiningManager.h>
#include <mining-manager/taxonomy-generation-submanager/TaxonomyRep.h>
#include <mining-manager/taxonomy-generation-submanager/TaxonomyGenerationSubManager.h>
namespace sf1r
{

extern int TOP_K_NUM;


void AggregatorManager::aggregateDistSearchInfo(DistKeywordSearchInfo& result, const std::vector<std::pair<workerid_t, DistKeywordSearchInfo> >& resultList)
{
    cout << "#[AggregatorManager::aggregateDistSearchInfo] " << resultList.size() << endl;

    size_t resultNum = resultList.size();

    for (size_t i = 0; i < resultNum; i++)
    {
        DistKeywordSearchInfo& wResult = const_cast<DistKeywordSearchInfo&>(resultList[i].second);

        DocumentFrequencyInProperties::iterator dfiter;
        for (dfiter = wResult.dfmap_.begin(); dfiter != wResult.dfmap_.end(); dfiter++)
        {
            const std::string& property = dfiter->first;

            ID_FREQ_MAP_T& df = wResult.dfmap_[property];
            ID_FREQ_UNORDERED_MAP_T::iterator iter_;
            for (iter_ = df.begin(); iter_ != df.end(); iter_++)
            {
                result.dfmap_[property][iter_->first] += iter_->second;
            }
        }

        CollectionTermFrequencyInProperties::iterator ctfiter;
        for (ctfiter = wResult.ctfmap_.begin(); ctfiter != wResult.ctfmap_.end(); ctfiter++)
        {
            const std::string& property = ctfiter->first;

            ID_FREQ_MAP_T& ctf = wResult.ctfmap_[property];
            ID_FREQ_UNORDERED_MAP_T::iterator iter_;
            for (iter_ = ctf.begin(); iter_ != ctf.end(); iter_++)
            {
                result.ctfmap_[property][iter_->first] += iter_->second;
            }
        }
    }
}

void AggregatorManager::aggregateSearchResult(DistKeywordSearchResult& result, const std::vector<std::pair<workerid_t, DistKeywordSearchResult> >& resultList)
{
    cout << "#[AggregatorManager::aggregateSearchResult] " << resultList.size() << endl;

    size_t workerNum = resultList.size();

    // only one result
    if (workerNum == 1)
    {
        result = resultList[0].second;
        result.topKWorkerIds_.resize(result.topKDocs_.size(), resultList[0].first);
        return;
    }

    // Set basic info
    result.analyzedQuery_ = resultList[0].second.analyzedQuery_;
    result.queryTermIdList_ = resultList[0].second.queryTermIdList_;
    result.propertyQueryTermList_ = resultList[0].second.propertyQueryTermList_;

    result.totalCount_ = 0;
    size_t overallResultCount = 0;
    bool hasCustomRankScore = false;
    for (size_t i = 0; i < workerNum; i++)
    {
        const DistKeywordSearchResult& wResult = resultList[i].second;
        //wResult.print();//:~

        result.totalCount_ += wResult.totalCount_;
        overallResultCount += wResult.topKDocs_.size();

        if (wResult.topKCustomRankScoreList_.size() > 0)
            hasCustomRankScore = true;
    }
    cout << "result.totalCount_: " << result.totalCount_ << ",  overallResultCount: " << overallResultCount<<endl;

    // set page info
    if (result.start_ >= overallResultCount)
    {
        result.count_ = 0;
        return;
    }
    if (result.start_ + result.count_ > overallResultCount)
    {
        result.count_ = overallResultCount - result.start_;
    }
    cout << "resultPage      start: " << result.start_ << ", count: " << result.count_ << endl;

    // reserve data size for topK docs
    size_t resultCount = overallResultCount < size_t(TOP_K_NUM) ? overallResultCount : TOP_K_NUM;
    result.topKDocs_.resize(resultCount);
    result.topKWorkerIds_.resize(resultCount);
    result.topKRankScoreList_.resize(resultCount);
    if (hasCustomRankScore)
        result.topKCustomRankScoreList_.resize(resultCount);

    // Prepare comparator for each sub result (compare by sort properties).
    DocumentComparator** docComparators = new DocumentComparator*[workerNum];
    for (size_t i = 0; i < workerNum; i++)
    {
        docComparators[i] = new DocumentComparator(resultList[i].second);
    }
    // Merge topK docs
    size_t maxi;
    size_t* iter = new size_t[workerNum];
    memset(iter, 0, sizeof(size_t)*workerNum);
    for (size_t cnt = 0; cnt < resultCount; cnt++)
    {
        // find doc which should be merged firstly from heads of multiple doc lists (sorted).
        maxi = size_t(-1);
        for (size_t i = 0; i < workerNum; i++)
        {
            const std::vector<docid_t>& subTopKDocs = resultList[i].second.topKDocs_;
            if (iter[i] >= subTopKDocs.size())
                continue;

            if (maxi == size_t(-1))
            {
                maxi = i;
                continue;
            }

            if (greaterThan(docComparators[i], iter[i], docComparators[maxi], iter[maxi]))
            {
                maxi = i;
            }
        }

        //cout << "maxi: "<< maxi<<", iter[maxi]: " << iter[maxi]<<endl;
        if (maxi == size_t(-1))
        {
            break;
        }

        // get a result
        const workerid_t& workerid = resultList[maxi].first;
        const DistKeywordSearchResult& wResult = resultList[maxi].second;

        result.topKDocs_[cnt] = wResult.topKDocs_[iter[maxi]];
        result.topKWorkerIds_[cnt] = workerid;
        result.topKRankScoreList_[cnt] = wResult.topKRankScoreList_[iter[maxi]];
        if (hasCustomRankScore && wResult.topKCustomRankScoreList_.size() > 0)
            result.topKCustomRankScoreList_[cnt] = wResult.topKCustomRankScoreList_[iter[maxi]];

        // next
        iter[maxi] ++;
    }

    delete[] iter;
    for (size_t i = 0; i < workerNum; i++)
    {
        delete docComparators[i];
    }
    delete docComparators;

    // TODO, merge Ontology info
    //result.onto_rep_;
    //result.groupRep_;
    //result.attrRep_;
}

void AggregatorManager::aggregateSummaryMiningResult(KeywordSearchResult& result, const std::vector<std::pair<workerid_t, KeywordSearchResult> >& resultList)
{
    cout << "#[AggregatorManager::aggregateSummaryMiningResult] " << resultList.size() << endl;

    aggregateSummaryResult(result, resultList);

    aggregateMiningResult(result, resultList);
}

void AggregatorManager::aggregateSummaryResult(KeywordSearchResult& result, const std::vector<std::pair<workerid_t, KeywordSearchResult> >& resultList)
{
    size_t subResultNum = resultList.size();

    if (subResultNum <= 0)
        return;

    size_t pageCount = 0;
    for (size_t i = 0; i < subResultNum; i++)
    {
        pageCount += resultList[i].second.count_;
    }
    if (result.count_ > pageCount)
    {
        result.count_ = pageCount;
    }
    else
    {
        pageCount = result.count_;
    }

    size_t displayPropertyNum = resultList[0].second.snippetTextOfDocumentInPage_.size(); //xxx
    size_t isSummaryOn = resultList[0].second.rawTextOfSummaryInPage_.size(); //xxx

    // initialize summary info for result
    result.snippetTextOfDocumentInPage_.resize(displayPropertyNum);
    result.fullTextOfDocumentInPage_.resize(displayPropertyNum);
    if (isSummaryOn)
        result.rawTextOfSummaryInPage_.resize(isSummaryOn);

    for (size_t dis = 0; dis < displayPropertyNum; dis++)
    {
        result.snippetTextOfDocumentInPage_[dis].resize(pageCount);
        result.fullTextOfDocumentInPage_[dis].resize(pageCount);
    }
    for (size_t dis = 0; dis < isSummaryOn; dis++)
    {
        result.rawTextOfSummaryInPage_[dis].resize(pageCount);
    }

    // merge
    // each sub result provided part of docs for result page, and they kept the doc postions
    // in topKDocs of result for that page.
    size_t curSub;
    size_t* iter = new size_t[subResultNum];
    memset(iter, 0, sizeof(size_t)*subResultNum);

    for (size_t i = 0, pos = result.start_; i < pageCount; i++, pos++)
    {
        curSub = size_t(-1);
        for (size_t sub = 0; sub < subResultNum; sub++)
        {
            const std::vector<size_t>& subTopKPosList = resultList[sub].second.topKPostionList_;
            if (iter[sub] < subTopKPosList.size() && subTopKPosList[iter[sub]] == pos)
            {
                curSub = sub;
                break;
            }
        }

        if (curSub == size_t(-1))
            break;
        //cout << "index,pos:" << i <<","<< pos <<"   curSub:"<< curSub<<"  iter[curSub]:" << iter[curSub]<<endl;

        // get a result
        const KeywordSearchResult& subResult = resultList[curSub].second;
        for (size_t dis = 0; dis < displayPropertyNum; dis++)
        {
            result.snippetTextOfDocumentInPage_[dis][i] = subResult.snippetTextOfDocumentInPage_[dis][iter[curSub]];
            result.fullTextOfDocumentInPage_[dis][i] = subResult.fullTextOfDocumentInPage_[dis][iter[curSub]];
        }
        for (size_t dis = 0; dis < isSummaryOn; dis++)
        {
            result.rawTextOfSummaryInPage_[dis][i] = subResult.rawTextOfSummaryInPage_[dis][iter[curSub]];
        }

        // next
        iter[curSub] ++;
    }

    delete[] iter;
}

void AggregatorManager::aggregateMiningResult(KeywordSearchResult& result, const std::vector<std::pair<workerid_t, KeywordSearchResult> >& resultList)
{
    if(!mining_manager_) return;
    std::cout<<"call mergeMiningResult"<<std::endl;
    boost::shared_ptr<TaxonomyGenerationSubManager> tg_manager = mining_manager_->GetTgManager();
    if(tg_manager)
    {
        std::vector<std::pair<uint32_t, idmlib::cc::CCInput32> > input_list;
        for(uint32_t i=0;i<resultList.size();i++)
        {
            const idmlib::cc::CCInput32& tg_input = resultList[i].second.tg_input;
            if(tg_input.concept_list.size()>0)
            {
                input_list.push_back(std::make_pair(resultList[i].first, tg_input) );
            }
        }
        if( input_list.size()>0)
        {
            std::vector<sf1r::wdocid_t> top_wdoclist;
            result.getTopKWDocs(top_wdoclist);
            idmlib::cc::CCInput64 input;
            std::cout<<"before merging, size : "<<input_list.size()<<std::endl;
            tg_manager->AggregateInput(top_wdoclist, input_list, input);
            std::cout<<"after merging, concept size : "<<input.concept_list.size()<<" , doc size : "<<input.doc_list.size()<<std::endl;
            TaxonomyRep taxonomyRep;
            if( tg_manager->GetResult(input, taxonomyRep, result.neList_) )
            {
                taxonomyRep.fill(result);
            }
        }
    }
}

void AggregatorManager::aggregateDocumentsResult(RawTextResultFromSIA& result, const std::vector<std::pair<workerid_t, RawTextResultFromSIA> >& resultList)
{
    cout << "#[AggregatorManager::aggregateDocumentsResult] " << resultList.size() << endl;

    size_t workerNum = resultList.size();

    // only one result
    if (workerNum == 1)
    {
        result = resultList[0].second;
        return;
    }

    result.idList_.clear();

    for (size_t w = 0; w < workerNum; w++)
    {
        //workerid_t workerid = resultList[w].first;
        const RawTextResultFromSIA& wResult = resultList[w].second;

        for (size_t i = 0; i < wResult.idList_.size(); i++)
        {
            if (result.idList_.empty())
            {
                size_t displayPropertySize = wResult.fullTextOfDocumentInPage_.size();
                result.fullTextOfDocumentInPage_.resize(displayPropertySize);
                result.snippetTextOfDocumentInPage_.resize(displayPropertySize);
                result.rawTextOfSummaryInPage_.resize(wResult.rawTextOfSummaryInPage_.size());
            }

            result.idList_.push_back(wResult.idList_[i]);

            for (size_t p = 0; p < wResult.fullTextOfDocumentInPage_.size(); p++)
            {
                result.fullTextOfDocumentInPage_[p].push_back(wResult.fullTextOfDocumentInPage_[p][i]);
                result.snippetTextOfDocumentInPage_[p].push_back(wResult.snippetTextOfDocumentInPage_[p][i]);
            }

            for (size_t s = 0; s <wResult.rawTextOfSummaryInPage_.size(); s++)
            {
                result.snippetTextOfDocumentInPage_[s].push_back(wResult.snippetTextOfDocumentInPage_[s][i]);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

bool AggregatorManager::splitSearchResultByWorkerid(const KeywordSearchResult& result, std::map<workerid_t, boost::shared_ptr<KeywordSearchResult> >& resultMap)
{
    const std::vector<uint32_t>& topKWorkerIds = result.topKWorkerIds_;

    // split docs of current page by worker
    boost::shared_ptr<KeywordSearchResult> subResult;
    for (size_t i = 0; i < topKWorkerIds.size(); i ++)
    {
        workerid_t curWorkerid = topKWorkerIds[i];
        if (resultMap.find(curWorkerid) == resultMap.end())
        {
            subResult.reset(new KeywordSearchResult());
            // xxx, here just copy info needed for getting summary, mining result.
            subResult->start_ = size_t(-1);
            subResult->count_ = 0;
            subResult->propertyQueryTermList_ = result.propertyQueryTermList_;
            resultMap.insert(std::make_pair(curWorkerid, subResult));
        }
        else
        {
            subResult = resultMap[curWorkerid];
        }

        subResult->topKDocs_.push_back(result.topKDocs_[i]);
        subResult->topKWorkerIds_.push_back(curWorkerid);

        if (i >= result.start_ && i < result.start_+result.count_)
        {
            subResult->topKPostionList_.push_back(i);
            if (subResult->start_ == size_t(-1))
            {
                subResult->start_ = subResult->topKDocs_.size() - 1;
            }
            subResult->count_ += 1;
        }
    }

    return true;
}

std::pair<bool, workerid_t> AggregatorManager::splitGetDocsActionItemByWorkerid(
        const GetDocumentsByIdsActionItem& actionItem,
        std::map<workerid_t, boost::shared_ptr<GetDocumentsByIdsActionItem> >& actionItemMap)
{
    cout << "#[AggregatorManager::splitGetDocsActionItemByWorkerid] " <<endl;

    std::vector<sf1r::docid_t> idList;
    std::vector<sf1r::workerid_t> workeridList;
    std::set<sf1r::workerid_t> ret =
            const_cast<GetDocumentsByIdsActionItem&>(actionItem).getDocWorkerIdLists(idList, workeridList);

    if (ret.size() == 0)
    {
        return std::pair<bool, workerid_t>(false, workerid_t(-1));
    }

    if (ret.size() == 1)
    {
        return std::pair<bool, workerid_t>(false, *ret.begin());
    }

    // split
    boost::shared_ptr<GetDocumentsByIdsActionItem> subActionItem;
    for (size_t i = 0; i < workeridList.size(); i++)
    {
        workerid_t& curWorkerid = workeridList[i];
        if (actionItemMap.find(curWorkerid) == actionItemMap.end())
        {
            subActionItem.reset(new GetDocumentsByIdsActionItem());
            *subActionItem = actionItem;
            subActionItem->idList_.clear();
            actionItemMap.insert(std::make_pair(curWorkerid, subActionItem));
        }
        else
        {
            subActionItem = actionItemMap[curWorkerid];
        }

        subActionItem->idList_.push_back(actionItem.idList_[i]);
    }

    return std::pair<bool, workerid_t>(true, workerid_t(-1));
}

}


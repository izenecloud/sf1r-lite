#include "SearchMerger.h"
#include "MergeComparator.h"

#include <common/ResultType.h>
#include <common/Utilities.h>
#include <mining-manager/MiningManager.h>
#include <mining-manager/taxonomy-generation-submanager/TaxonomyRep.h>
#include <mining-manager/taxonomy-generation-submanager/TaxonomyGenerationSubManager.h>

#include <algorithm> // min
#include <vector>

namespace sf1r
{

void SearchMerger::getDistSearchInfo(const net::aggregator::WorkerResults<DistKeywordSearchInfo>& workerResults, DistKeywordSearchInfo& mergeResult)
{
    LOG(INFO) << "#[SearchMerger::getDistSearchInfo] " << workerResults.size() << endl;

    size_t resultNum = workerResults.size();

    for (size_t i = 0; i < resultNum; i++)
    {
        DistKeywordSearchInfo& wResult = const_cast<DistKeywordSearchInfo&>(workerResults.result(i));

        DocumentFrequencyInProperties::iterator dfiter;
        for (dfiter = wResult.dfmap_.begin(); dfiter != wResult.dfmap_.end(); dfiter++)
        {
            const std::string& property = dfiter->first;

            ID_FREQ_MAP_T& df = wResult.dfmap_[property];
            ID_FREQ_UNORDERED_MAP_T::iterator iter_;
            for (iter_ = df.begin(); iter_ != df.end(); iter_++)
            {
                mergeResult.dfmap_[property][iter_->first] += iter_->second;
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
                mergeResult.ctfmap_[property][iter_->first] += iter_->second;
            }
        }
    }
}

void SearchMerger::getDistSearchResult(const net::aggregator::WorkerResults<KeywordSearchResult>& workerResults, KeywordSearchResult& mergeResult)
{
    LOG(INFO) << "#[SearchMerger::getDistSearchResult] " << workerResults.size() << endl;

    size_t workerNum = workerResults.size();
    const KeywordSearchResult& result0 = workerResults.result(0);

    // only one result
    if (workerNum == 1)
    {
        mergeResult = result0;
        mergeResult.topKWorkerIds_.resize(mergeResult.topKDocs_.size(), workerResults.workerId(0));
        return;
    }

    // Set basic info
    mergeResult.collectionName_ = result0.collectionName_;
    mergeResult.encodingType_ = result0.encodingType_;
    mergeResult.rawQueryString_ = result0.rawQueryString_;
    mergeResult.start_ = result0.start_;
    mergeResult.count_ = result0.count_;
    mergeResult.analyzedQuery_ = result0.analyzedQuery_;
    mergeResult.queryTermIdList_ = result0.queryTermIdList_;
    mergeResult.propertyQueryTermList_ = result0.propertyQueryTermList_;
    mergeResult.totalCount_ = 0;
    size_t totalTopKCount = 0;
    bool hasCustomRankScore = false;
    float rangeLow = numeric_limits<float>::max(), rangeHigh = numeric_limits<float>::min();
    for (size_t i = 0; i < workerNum; i++)
    {
        const KeywordSearchResult& wResult = workerResults.result(i);
        //wResult.print();

        mergeResult.totalCount_ += wResult.totalCount_;
        totalTopKCount += wResult.topKDocs_.size();

        if (wResult.topKCustomRankScoreList_.size() > 0)
            hasCustomRankScore = true;

        if (wResult.propertyRange_.lowValue_ < rangeLow)
        {
            rangeLow = wResult.propertyRange_.lowValue_;
        }
        if (wResult.propertyRange_.highValue_ > rangeHigh)
        {
            rangeHigh = wResult.propertyRange_.highValue_;
        }
        mergeResult.groupRep_.merge(wResult.groupRep_);
        std::map<std::string,unsigned>::const_iterator cit = wResult.counterResults_.begin();
        for(; cit != wResult.counterResults_.end(); ++cit)
        {
            mergeResult.counterResults_[cit->first] += cit->second;
        }
    }
    mergeResult.propertyRange_.lowValue_ = rangeLow;
    mergeResult.propertyRange_.highValue_ = rangeHigh;

    size_t endOffset = mergeResult.start_ + mergeResult.count_;
    size_t endTopK = Utilities::roundUp(endOffset, TOP_K_NUM);
    size_t topKCount = std::min(endTopK, totalTopKCount);

    LOG(INFO) << "SearchMerger topKCount: " << topKCount << ", totalTopKCount: " << totalTopKCount << endl;

    mergeResult.topKDocs_.resize(topKCount);
    mergeResult.topKWorkerIds_.resize(topKCount);
    mergeResult.topKRankScoreList_.resize(topKCount);

    if (hasCustomRankScore)
        mergeResult.topKCustomRankScoreList_.resize(topKCount);

    // Prepare comparator for each sub result (compare by sort properties).
    DocumentComparator** docComparators = new DocumentComparator*[workerNum];
    for (size_t i = 0; i < workerNum; i++)
    {
        docComparators[i] = new DocumentComparator(workerResults.result(i));
    }
    // Merge topK docs (use Loser Tree to optimize k-way merge)
    size_t maxi;
    size_t* iter = new size_t[workerNum];
    memset(iter, 0, sizeof(size_t)*workerNum);
    for (size_t cnt = 0; cnt < topKCount; cnt++)
    {
        // find doc which should be merged firstly from heads of multiple doc lists (sorted).
        maxi = size_t(-1);
        for (size_t i = 0; i < workerNum; i++)
        {
            const std::vector<docid_t>& subTopKDocs = workerResults.result(i).topKDocs_;
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
        const workerid_t& workerid = workerResults.workerId(maxi);
        const KeywordSearchResult& wResult = workerResults.result(maxi);

        mergeResult.topKDocs_[cnt] = wResult.topKDocs_[iter[maxi]];
        mergeResult.topKWorkerIds_[cnt] = workerid;
        mergeResult.topKRankScoreList_[cnt] = wResult.topKRankScoreList_[iter[maxi]];
        if (hasCustomRankScore && wResult.topKCustomRankScoreList_.size() > 0)
            mergeResult.topKCustomRankScoreList_[cnt] = wResult.topKCustomRankScoreList_[iter[maxi]];

        // next
        iter[maxi] ++;
    }

    delete[] iter;
    for (size_t i = 0; i < workerNum; i++)
    {
        delete docComparators[i];
    }
    delete docComparators;
    LOG(INFO) << "#[SearchMerger::getDistSearchResult] finished";
}

void SearchMerger::getSummaryResult(const net::aggregator::WorkerResults<KeywordSearchResult>& workerResults, KeywordSearchResult& mergeResult)
{
    const size_t workerNum = workerResults.size();

    if (workerNum == 0)
        return;

    for(size_t workerId = 0; workerId < workerNum; ++workerId)
    {
        if(!workerResults.result(workerId).error_.empty())
        {
            mergeResult.error_ = workerResults.result(workerId).error_;
            return;
        }
    }

    LOG(INFO) << "#[SearchMerger::getSummaryResult] begin";
    size_t pageCount = mergeResult.count_;
    size_t displayPropertyNum = workerResults.result(0).snippetTextOfDocumentInPage_.size();
    size_t isSummaryOn = workerResults.result(0).rawTextOfSummaryInPage_.size();

    // initialize summary info for result
    mergeResult.snippetTextOfDocumentInPage_.resize(displayPropertyNum);
    mergeResult.fullTextOfDocumentInPage_.resize(displayPropertyNum);
    if (isSummaryOn)
        mergeResult.rawTextOfSummaryInPage_.resize(isSummaryOn);

    for (size_t dis = 0; dis < displayPropertyNum; dis++)
    {
        mergeResult.snippetTextOfDocumentInPage_[dis].resize(pageCount);
        mergeResult.fullTextOfDocumentInPage_[dis].resize(pageCount);
    }
    for (size_t dis = 0; dis < isSummaryOn; dis++)
    {
        mergeResult.rawTextOfSummaryInPage_[dis].resize(pageCount);
    }

    std::vector<std::size_t> workerOffsetVec(workerNum);
    for (size_t i = 0; i < pageCount; ++i)
    {
        std::size_t curWorker = 0;
        while (curWorker < workerNum)
        {
            const std::size_t workerOffset = workerOffsetVec[curWorker];
            const std::vector<std::size_t>& pageOffsetList = workerResults.result(curWorker).pageOffsetList_;

            if (workerOffset < pageOffsetList.size() && pageOffsetList[workerOffset] == i)
                break;

            ++curWorker;
        }

        if (curWorker == workerNum)
            break;

        const KeywordSearchResult& workerResult = workerResults.result(curWorker);
        std::size_t& workerOffset = workerOffsetVec[curWorker];

        for (size_t dis = 0; dis < displayPropertyNum; ++dis)
        {
            if (workerResult.snippetTextOfDocumentInPage_.size() > dis && workerResult.snippetTextOfDocumentInPage_[dis].size() > workerOffset)
                mergeResult.snippetTextOfDocumentInPage_[dis][i] = workerResult.snippetTextOfDocumentInPage_[dis][workerOffset];
            if (workerResult.fullTextOfDocumentInPage_.size() > dis && workerResult.fullTextOfDocumentInPage_[dis].size() > workerOffset)
                mergeResult.fullTextOfDocumentInPage_[dis][i] = workerResult.fullTextOfDocumentInPage_[dis][workerOffset];
        }
        for (size_t dis = 0; dis < isSummaryOn; ++dis)
        {
            if (workerResult.rawTextOfSummaryInPage_.size() > dis && workerResult.rawTextOfSummaryInPage_[dis].size() > workerOffset)
                mergeResult.rawTextOfSummaryInPage_[dis][i] = workerResult.rawTextOfSummaryInPage_[dis][workerOffset];
        }

        ++workerOffset;
    }
    LOG(INFO) << "#[SearchMerger::getSummaryResult] end";
}

void SearchMerger::getSummaryMiningResult(const net::aggregator::WorkerResults<KeywordSearchResult>& workerResults, KeywordSearchResult& mergeResult)
{
    LOG(INFO) << "#[SearchMerger::getSummaryMiningResult] " << workerResults.size() << endl;

    getSummaryResult(workerResults, mergeResult);

    getMiningResult(workerResults, mergeResult);
    LOG(INFO) << "#[SearchMerger::getSummaryMiningResult] end";
}

void SearchMerger::getMiningResult(const net::aggregator::WorkerResults<KeywordSearchResult>& workerResults, KeywordSearchResult& mergeResult)
{
    if(!miningManager_) return;
    LOG(INFO) <<"call mergeMiningResult"<<std::endl;
    boost::shared_ptr<TaxonomyGenerationSubManager> tg_manager = miningManager_->GetTgManager();
    if(tg_manager)
    {
        std::vector<std::pair<uint32_t, idmlib::cc::CCInput32> > input_list;
        for(uint32_t i=0;i<workerResults.size();i++)
        {
            const idmlib::cc::CCInput32& tg_input = workerResults.result(i).tg_input;
            if(tg_input.concept_list.size()>0)
            {
                input_list.push_back(std::make_pair(workerResults.workerId(i), tg_input) );
            }
        }
        if( input_list.size()>0)
        {
            std::vector<sf1r::wdocid_t> top_wdoclist;
            mergeResult.getTopKWDocs(top_wdoclist);
            idmlib::cc::CCInput64 input;
            LOG(INFO)<<"before merging, size : "<<input_list.size()<<std::endl;
            tg_manager->AggregateInput(top_wdoclist, input_list, input);
            LOG(INFO)<<"after merging, concept size : "<<input.concept_list.size()<<" , doc size : "<<input.doc_list.size()<<std::endl;
            TaxonomyRep taxonomyRep;
            if( tg_manager->GetResult(input, taxonomyRep, mergeResult.neList_) )
            {
                taxonomyRep.fill(mergeResult);
            }
        }
    }
}

void SearchMerger::getDocumentsByIds(const net::aggregator::WorkerResults<RawTextResultFromSIA>& workerResults, RawTextResultFromSIA& mergeResult)
{
    LOG(INFO) << "#[SearchMerger::getDocumentsByIds] " << workerResults.size() << endl;

    size_t workerNum = workerResults.size();

    // only one result
    if (workerNum == 1)
    {
        mergeResult = workerResults.result(0);
        mergeResult.workeridList_.resize(mergeResult.idList_.size(), workerResults.workerId(0));
        return;
    }

    mergeResult.idList_.clear();

    for (size_t w = 0; w < workerNum; w++)
    {
        workerid_t workerid = workerResults.workerId(w);
        const RawTextResultFromSIA& wResult = workerResults.result(w);

        for (size_t i = 0; i < wResult.idList_.size(); i++)
        {
            if (mergeResult.idList_.empty())
            {
                size_t displayPropertySize = wResult.fullTextOfDocumentInPage_.size();
                mergeResult.fullTextOfDocumentInPage_.resize(displayPropertySize);
                mergeResult.snippetTextOfDocumentInPage_.resize(displayPropertySize);
                mergeResult.rawTextOfSummaryInPage_.resize(wResult.rawTextOfSummaryInPage_.size());
            }

            // id and corresponding workerid
            mergeResult.idList_.push_back(wResult.idList_[i]);
            mergeResult.workeridList_.push_back(workerid);

            for (size_t p = 0; p < wResult.fullTextOfDocumentInPage_.size(); p++)
            {
                mergeResult.fullTextOfDocumentInPage_[p].push_back(wResult.fullTextOfDocumentInPage_[p][i]);
                mergeResult.snippetTextOfDocumentInPage_[p].push_back(wResult.snippetTextOfDocumentInPage_[p][i]);
            }

            for (size_t s = 0; s <wResult.rawTextOfSummaryInPage_.size(); s++)
            {
                mergeResult.snippetTextOfDocumentInPage_[s].push_back(wResult.snippetTextOfDocumentInPage_[s][i]);
            }
        }
    }
    LOG(INFO) << "#[SearchMerger::getDocumentsByIds] end"<< endl;
}

void SearchMerger::getInternalDocumentId(const net::aggregator::WorkerResults<uint64_t>& workerResults, uint64_t& mergeResult)
{
    mergeResult = 0;

    size_t workerNum = workerResults.size();
    for (size_t w = 0; w < workerNum; w++)
    {
        // Assume and DOCID should be unique in global space
        uint64_t wResult = workerResults.result(w);
        if (wResult != 0)
        {
            mergeResult = net::aggregator::Util::GetWDocId(workerResults.workerId(w), (uint32_t)wResult);
            return;
        }
    }
}

void SearchMerger::clickGroupLabel(const net::aggregator::WorkerResults<bool>& workerResults, bool& mergeResult)
{
    mergeResult = false;

    size_t workerNum = workerResults.size();
    for (size_t i = 0; i < workerNum; ++i)
    {
        if (workerResults.result(i))
        {
            mergeResult = true;
            return;
        }
    }
}

void SearchMerger::splitSearchResultByWorkerid(const KeywordSearchResult& totalResult, std::map<workerid_t, KeywordSearchResult>& resultMap)
{
    std::size_t i = 0;
    std::size_t topKOffset = totalResult.start_;

    while (i < totalResult.count_)
    {
        workerid_t curWorkerId = totalResult.topKWorkerIds_[topKOffset];
        KeywordSearchResult& workerResult = resultMap[curWorkerId];

        if (workerResult.count_ == 0)
        {
            workerResult.propertyQueryTermList_ = totalResult.propertyQueryTermList_;
        }

        workerResult.topKDocs_.push_back(totalResult.topKDocs_[topKOffset]);
        workerResult.topKWorkerIds_.push_back(curWorkerId);
        workerResult.pageOffsetList_.push_back(i);
        ++workerResult.count_;

        ++i;
        ++topKOffset;
    }
}

bool SearchMerger::splitGetDocsActionItemByWorkerid(
    const GetDocumentsByIdsActionItem& actionItem,
    std::map<workerid_t, GetDocumentsByIdsActionItem>& actionItemMap)
{
    if (actionItem.idList_.empty())
        return false;

    std::vector<sf1r::docid_t> idList;
    std::vector<sf1r::workerid_t> workeridList;
    actionItem.getDocWorkerIdLists(idList, workeridList);

    // split
    for (size_t i = 0; i < workeridList.size(); i++)
    {
        workerid_t& curWorkerid = workeridList[i];
        GetDocumentsByIdsActionItem& subActionItem = actionItemMap[curWorkerid];

        if (subActionItem.idList_.empty())
        {
            subActionItem = actionItem;
            subActionItem.idList_.clear();
        }

        subActionItem.idList_.push_back(actionItem.idList_[i]);
    }

    return true;
}

void SearchMerger::HookDistributeRequestForSearch(const net::aggregator::WorkerResults<bool>& workerResults, bool& mergeResult)
{
    mergeResult = true;
    size_t workerNum = workerResults.size();
    for (size_t i = 0; i < workerNum; ++i)
    {
        if (!workerResults.result(i))
        {
            mergeResult = false;
            return;
        }
    }
}

} // namespace sf1r

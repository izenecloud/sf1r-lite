#include "SearchThreadWorker.h"
#include "SearchManagerPreProcessor.h"
#include "SearchThreadParam.h"
#include "ScoreDocEvaluator.h"
#include "QueryBuilder.h"
#include "CombinedDocumentIterator.h"
#include "FilterDocumentIterator.h"
#include "AllDocumentIterator.h"
#include "CustomRankDocumentIterator.h"
#include "HitQueue.h"

#include <common/PropSharedLockSet.h>
#include <bundles/index/IndexBundleConfiguration.h>
#include <document-manager/DocumentManager.h>
#include <index-manager/IndexManager.h>
#include <ranking-manager/RankingManager.h>
#include <mining-manager/group-manager/GroupFilterBuilder.h>
#include <mining-manager/group-manager/GroupFilter.h>
#include <mining-manager/product-scorer/RelevanceScorer.h>
#include <mining-manager/custom-rank-manager/CustomRankManager.h>

#include <memory> // auto_ptr
#include <limits>
#include <boost/bind.hpp>

using namespace sf1r;

SearchThreadWorker::SearchThreadWorker(
    const IndexBundleConfiguration& config,
    const boost::shared_ptr<DocumentManager>& documentManager,
    const boost::shared_ptr<IndexManager>& indexManager,
    const boost::shared_ptr<RankingManager>& rankingManager,
    SearchManagerPreProcessor& preprocessor,
    QueryBuilder& queryBuilder)
    : config_(config)
    , documentManagerPtr_(documentManager)
    , indexManagerPtr_(indexManager)
    , rankingManagerPtr_(rankingManager)
    , preprocessor_(preprocessor)
    , queryBuilder_(queryBuilder)
    , groupFilterBuilder_(NULL)
    , customRankManager_(NULL)
{
    rankingManagerPtr_->getPropertyWeightMap(propertyWeightMap_);
}

void SearchThreadWorker::setGroupFilterBuilder(const faceted::GroupFilterBuilder* builder)
{
    groupFilterBuilder_ = builder;
}

void SearchThreadWorker::setCustomRankManager(CustomRankManager* customRankManager)
{
    customRankManager_ = customRankManager;
}

bool SearchThreadWorker::search(SearchThreadParam& param)
{
    CREATE_PROFILER(preparedociter, "SearchThreadWorker", "search: build doc iterator");
    CREATE_PROFILER(preparerank, "SearchThreadWorker", "search: prepare ranker");

    const SearchKeywordOperation& actionOperation = *param.actionOperation;
    LOG(INFO) << "search in thread worker begin ------------ " << time(NULL);

    unsigned int collectionId = 1;
    std::vector<std::string> indexPropertyList(actionOperation.actionItem_.searchPropertyList_);
    START_PROFILER(preparedociter)
    unsigned indexPropertySize = indexPropertyList.size();
    std::vector<propertyid_t> indexPropertyIdList(indexPropertySize);

    preprocessor_.preparePropertyList(indexPropertyList,
                                      indexPropertyIdList,
                                      boost::bind(&QueryBuilder::getPropertyIdByName, &queryBuilder_, _1));

    // build term index maps
    const std::map<std::string,PropertyTermInfo>& propertyTermInfoMap =
        actionOperation.getPropertyTermInfoMap();
    std::vector<std::map<termid_t, unsigned> > termIndexMaps(indexPropertySize);
    preprocessor_.preparePropertyTermIndex(propertyTermInfoMap,
                                           indexPropertyList, termIndexMaps);


    TextRankingType& pTextRankingType = actionOperation.actionItem_.rankingType_;
    bool isWandStrategy = actionOperation.actionItem_.searchingMode_.mode_ == SearchingMode::WAND;
    bool isTFIDFModel = (pTextRankingType == RankingType::BM25 ||
                         config_.rankingManagerConfig_.rankingConfigUnit_.textRankingModel_ == RankingType::BM25);
    bool isWandSearch = isWandStrategy && isTFIDFModel && !actionOperation.isPhraseOrWildcardQuery_;
    bool useOriginalQuery = actionOperation.actionItem_.searchingMode_.useOriginalQuery_;

    std::vector<boost::shared_ptr<PropertyRanker> > propertyRankers;
    rankingManagerPtr_->createPropertyRankers(pTextRankingType, indexPropertySize, propertyRankers);
    bool readTermPosition = propertyRankers[0]->requireTermPosition();

    std::auto_ptr<DocumentIterator> scoreDocIterPtr;
    MultiPropertyScorer* pMultiPropertyIterator = NULL;
    WANDDocumentIterator* pWandDocIterator = NULL;

    std::vector<QueryFiltering::FilteringTreeValue>& filtingTreeList =
        actionOperation.actionItem_.filteringTreeList_;
    boost::shared_ptr<IndexManager::FilterBitmapT> pFilterIdSet;

    // when query is "*"
    const bool isFilterQuery =
        actionOperation.rawQueryTree_->type_ == QueryTree::FILTER_QUERY;

    try
    {
        if (!filtingTreeList.empty())
            queryBuilder_.prepare_filter(filtingTreeList, pFilterIdSet);
        if (isFilterQuery == false)
        {
            if (isWandSearch)
            {
                pWandDocIterator = queryBuilder_.prepare_wand_dociterator(
                                       actionOperation,
                                       collectionId,
                                       propertyWeightMap_,
                                       indexPropertyList,
                                       indexPropertyIdList,
                                       readTermPosition,
                                       termIndexMaps);
                scoreDocIterPtr.reset(pWandDocIterator);
            }
            else
            {
                pMultiPropertyIterator = queryBuilder_.prepare_dociterator(
                                             actionOperation,
                                             collectionId,
                                             propertyWeightMap_,
                                             indexPropertyList,
                                             indexPropertyIdList,
                                             readTermPosition,
                                             termIndexMaps);
                scoreDocIterPtr.reset(pMultiPropertyIterator);
            }
        }
    }
    catch (std::exception& e)
    {
        return false;
    }
    boost::shared_ptr<CombinedDocumentIterator> pDocIterator(
        new CombinedDocumentIterator());
    if (pFilterIdSet)
    {
        ///1. Search Filter
        ///2. Select * WHERE    (FilterQuery)
        TermDocFreqs* pFilterTermDocFreqs = new IndexManager::FilterTermDocFreqsT(pFilterIdSet);
        FilterDocumentIterator* pFilterIterator = new FilterDocumentIterator(pFilterTermDocFreqs);
        pDocIterator->add(pFilterIterator);
    }

    STOP_PROFILER(preparedociter)

    try
    {
        preprocessor_.prepareSorterCustomRanker(actionOperation,
                                                param.pSorter,
                                                param.customRanker);
    }
    catch (std::exception& e)
    {
        return false;
    }

    PropSharedLockSet propSharedLockSet;
    ///sortby
    if (param.pSorter)
    {
        param.scoreItemQueue.reset(new PropertySortedHitQueue(param.pSorter,
                                                              param.heapSize,
                                                              propSharedLockSet));
    }
    else
    {
        param.scoreItemQueue.reset(new ScoreSortedHitQueue(param.heapSize));
    }

    DocumentIterator* pScoreDocIterator = scoreDocIterPtr.get();
    if (isFilterQuery == false)
    {
        pScoreDocIterator = combineCustomDocIterator_(
            actionOperation.actionItem_, scoreDocIterPtr.release());

        if (pScoreDocIterator == NULL)
        {
            const std::string& query =
                actionOperation.actionItem_.env_.queryString_;

            LOG(INFO) << "empty search result for query [" << query << "]";
            return false;
        }

        pDocIterator->add(pScoreDocIterator);
    }
    if (isFilterQuery && !pFilterIdSet)
    {
        //SELECT * , and filter is null
        if (param.pSorter)
        {
            ///SELECT * ORDER BY
            unsigned maxDoc = documentManagerPtr_->getMaxDocId();
            if (maxDoc == 0)
                return false;
            boost::shared_ptr<BitVector> pDelFilter(indexManagerPtr_->getBTreeIndexer()->getFilter());
            AllDocumentIterator* pFilterIterator = NULL;
            if (pDelFilter)
            {
                pFilterIdSet.reset(new IndexManager::FilterBitmapT);
                pDelFilter->compressed(*pFilterIdSet);
                TermDocFreqs* pDelTermDocFreqs = new IndexManager::FilterTermDocFreqsT(pFilterIdSet);
                pFilterIterator = new AllDocumentIterator(pDelTermDocFreqs, maxDoc);
            }
            else
            {
                pFilterIterator = new AllDocumentIterator(maxDoc);
            }
            pDocIterator->add(pFilterIterator);
        }
        else
        {
            return false;
        }
    }
    START_PROFILER(preparerank)

    ///prepare data for rankingmanager;
    DocumentFrequencyInProperties dfmap;
    CollectionTermFrequencyInProperties ctfmap;
    MaxTermFrequencyInProperties maxtfmap;

    // until now all the pDocIterators is the same, so we can just compute the df, ctf and maxtf use
    // any one.
    DistKeywordSearchInfo& distSearchInfo = *param.distSearchInfo;
    if (distSearchInfo.effective_)
    {
        if (distSearchInfo.isOptionGatherInfo())
        {
            pDocIterator->df_cmtf(dfmap, ctfmap, maxtfmap);
            distSearchInfo.dfmap_.swap(dfmap);
            distSearchInfo.ctfmap_.swap(ctfmap);
            distSearchInfo.maxtfmap_.swap(maxtfmap);
            return true;
        }
        else if (distSearchInfo.isOptionCarriedInfo())
        {
            dfmap = distSearchInfo.dfmap_;
            ctfmap = distSearchInfo.ctfmap_;
            maxtfmap = distSearchInfo.maxtfmap_;
        }
    }
    else
    {
        pDocIterator->df_cmtf(dfmap, ctfmap, maxtfmap);
    }
    std::vector<RankQueryProperty> rankQueryProperties(indexPropertyList.size());

    queryBuilder_.post_prepare_ranker_(
        "",
        indexPropertyList,
        indexPropertyList.size(),
        propertyTermInfoMap,
        dfmap,
        ctfmap,
        maxtfmap,
        readTermPosition,
        rankQueryProperties,
        propertyRankers);

    UpperBoundInProperties ubmap;
    if (pWandDocIterator)
    {
        for (size_t i = 0; i < indexPropertyList.size(); ++i)
        {
            const std::string& currentProperty = indexPropertyList[i];
            ID_FREQ_MAP_T& ub = ubmap[currentProperty];
            propertyRankers[i]->calculateTermUBs(rankQueryProperties[i], ub);
        }
        pWandDocIterator->set_ub(useOriginalQuery, ubmap);
        pWandDocIterator->init_threshold(actionOperation.actionItem_.searchingMode_.threshold_);
    }

    STOP_PROFILER(preparerank)

    boost::scoped_ptr<faceted::GroupFilter> groupFilter;
    if (groupFilterBuilder_)
    {
        groupFilter.reset(
            groupFilterBuilder_->createFilter(actionOperation.actionItem_.groupParam_,
                                              propSharedLockSet));
    }

    ProductScorer* relevanceScorer = NULL;
    if (pScoreDocIterator)
    {
        relevanceScorer = new RelevanceScorer(*pScoreDocIterator,
                                              rankQueryProperties,
                                              propertyRankers);
    }

    ProductScorer* productScorer = preprocessor_.createProductScorer(
        actionOperation.actionItem_, propSharedLockSet, relevanceScorer);

    ScoreDocEvaluator scoreDocEvaluator(productScorer, param.customRanker);
    try
    {
        time_t start_search = time(NULL);
        bool ret = doSearch_(param,
                             pDocIterator.get(),
                             groupFilter.get(),
                             scoreDocEvaluator,
                             propSharedLockSet);

        if (time(NULL) - start_search > 5)
            LOG(INFO) << "dosearch cost too long, " << start_search << ", " << time(NULL);

        if (groupFilter)
        {
            groupFilter->getGroupRep(param.groupRep, param.attrRep);
        }
        return ret;
    }
    catch (std::exception& e)
    {
        return false;
    }
    return false;
}

DocumentIterator* SearchThreadWorker::combineCustomDocIterator_(
    const KeywordSearchActionItem& actionItem,
    DocumentIterator* originDocIterator)
{
    if (customRankManager_ &&
        preprocessor_.isNeedCustomDocIterator(actionItem))
    {
        const std::string& query = actionItem.env_.queryString_;
        CustomRankDocId customDocId;

        if (!customRankManager_->getCustomValue(query, customDocId) ||
            customDocId.empty())
        {
            return originDocIterator;
        }

        DocumentIterator* pCustomDocIter =
            new CustomRankDocumentIterator(customDocId);

        if (originDocIterator)
        {
            pCustomDocIter->add(originDocIterator);
        }

        return pCustomDocIter;
    }

    return originDocIterator;
}

bool SearchThreadWorker::doSearch_(
    SearchThreadParam& param,
    CombinedDocumentIterator* pDocIterator,
    faceted::GroupFilter* groupFilter,
    ScoreDocEvaluator& scoreDocEvaluator,
    PropSharedLockSet& propSharedLockSet)
{
    CREATE_PROFILER(computerankscore, "SearchThreadWorker", "doSearch_: overall time for scoring a doc");
    CREATE_PROFILER(inserttoqueue, "SearchThreadWorker", "doSearch_: overall time for inserting to result queue");

    const SearchKeywordOperation& actionOperation = *param.actionOperation;

    typedef boost::shared_ptr<NumericPropertyTableBase> NumericPropertyTablePtr;
    const std::string& rangePropertyName = actionOperation.actionItem_.rangePropertyName_;
    NumericPropertyTablePtr rangePropertyTable;
    float lowValue = std::numeric_limits<float>::max();
    float highValue = -lowValue;

    if (!rangePropertyName.empty())
    {
        rangePropertyTable = documentManagerPtr_->getNumericPropertyTable(rangePropertyName);
        propSharedLockSet.insertSharedLock(rangePropertyTable.get());
    }

    std::vector<NumericPropertyTablePtr> counterTables;
    std::vector<uint32_t> counterValues;
    const std::vector<std::string>& counterList = actionOperation.actionItem_.counterList_;
    unsigned counterSize = counterList.size();
    unsigned ii = 0;
    if (counterSize)
    {
        counterTables.resize(counterSize);
        counterValues.resize(counterSize);
        for(; ii < counterSize; ++ii)
        {
            counterTables[ii] = documentManagerPtr_->getNumericPropertyTable(counterList[ii]);
            propSharedLockSet.insertSharedLock(counterTables[ii].get());
        }
    }

    pDocIterator->skipTo(param.docIdBegin);

    do
    {

        docid_t curDocId = pDocIterator->doc();

        if (curDocId >= param.docIdEnd)
            break;

        if (groupFilter && !groupFilter->test(curDocId))
            continue;

        if (rangePropertyTable)
        {
            float docPropertyValue = 0;
            if (rangePropertyTable->getFloatValue(curDocId, docPropertyValue, false))
            {
                if (docPropertyValue < lowValue)
                {
                    lowValue = docPropertyValue;
                }

                if (docPropertyValue > highValue)
                {
                    highValue = docPropertyValue;
                }
            }
        }

        /// COUNT(PropertyName)  semantics
        if (counterSize)
        {
            for (ii = 0; ii < counterSize; ++ii)
            {
                int32_t value;
                if (counterTables[ii]->getInt32Value(curDocId,value, false))
                {
                    counterValues[ii] += value;
                }
            }
        }

        ScoreDoc scoreItem(curDocId);

        START_PROFILER(computerankscore)

        ++param.totalCount;
        scoreDocEvaluator.evaluate(scoreItem);

        STOP_PROFILER(computerankscore)

        START_PROFILER(inserttoqueue)
        param.scoreItemQueue->insert(scoreItem);
        STOP_PROFILER(inserttoqueue)

    }
    while (pDocIterator->next());

    if (rangePropertyTable && lowValue <= highValue)
    {
        param.propertyRange.highValue_ = highValue;
        param.propertyRange.lowValue_ = lowValue;
    }
    if (counterSize)
    {
        for(ii = 0; ii < counterSize; ++ii)
        {
            const std::string& propName = counterList[ii];
            param.counterResults[propName] = counterValues[ii];
        }
    }
    return true;
}

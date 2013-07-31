/**
 * @file SearchWorkerController.h
 * @brief handle search request to worker server
 * @author Zhongxia Li, Jun Jiang
 * @date 2012-03-21
 */

#ifndef SEARCH_WORKER_CONTROLLER_H_
#define SEARCH_WORKER_CONTROLLER_H_

#include "Sf1WorkerController.h"
#include <aggregator-manager/SearchWorker.h>

#include <boost/shared_ptr.hpp>

namespace sf1r
{

class SearchWorkerController : public Sf1WorkerController
{
public:
    virtual bool addWorkerHandler(net::aggregator::WorkerRouter& router)
    {
        ADD_WORKER_HANDLER_BEGIN(SearchWorkerController, router)

        ADD_WORKER_HANDLER(getDistSearchInfo)
        ADD_WORKER_HANDLER(getDistSearchResult)
        ADD_WORKER_HANDLER(getSummaryResult)
        ADD_WORKER_HANDLER(getSummaryMiningResult)
        ADD_WORKER_HANDLER(getDocumentsByIds)
        ADD_WORKER_HANDLER(getInternalDocumentId)
        ADD_WORKER_HANDLER(clickGroupLabel)
        ADD_WORKER_HANDLER(visitDoc)
        ADD_WORKER_HANDLER(getSimilarDocIdList)
        ADD_WORKER_HANDLER(HookDistributeRequestForSearch)
        ADD_WORKER_HANDLER(GetSummarizationByRawKey)
        ADD_WORKER_HANDLER(getLabelListByDocId)
        ADD_WORKER_HANDLER(getLabelListWithSimByDocId)

        ADD_WORKER_HANDLER_END()
    }

    WORKER_CONTROLLER_METHOD_2(getDistSearchInfo, searchWorker_->getDistSearchInfo, KeywordSearchActionItem, DistKeywordSearchInfo)

    WORKER_CONTROLLER_METHOD_2(getDistSearchResult, searchWorker_->getDistSearchResult, KeywordSearchActionItem, KeywordSearchResult)

    WORKER_CONTROLLER_METHOD_2(getSummaryResult, searchWorker_->getSummaryResult, KeywordSearchActionItem, KeywordSearchResult)

    WORKER_CONTROLLER_METHOD_2(getSummaryMiningResult, searchWorker_->getSummaryMiningResult, KeywordSearchActionItem, KeywordSearchResult)

    WORKER_CONTROLLER_METHOD_2(getDocumentsByIds, searchWorker_->getDocumentsByIds, GetDocumentsByIdsActionItem, RawTextResultFromSIA)

    WORKER_CONTROLLER_METHOD_2(getInternalDocumentId, searchWorker_->getInternalDocumentId, uint128_t, uint64_t)

    WORKER_CONTROLLER_METHOD_2(clickGroupLabel, searchWorker_->clickGroupLabel, ClickGroupLabelActionItem, bool)

    WORKER_CONTROLLER_METHOD_2(visitDoc, searchWorker_->visitDoc, uint32_t, bool)

    WORKER_CONTROLLER_METHOD_3(getSimilarDocIdList, searchWorker_->getSimilarDocIdList, uint64_t, uint32_t, SimilarDocIdListType)

    WORKER_CONTROLLER_METHOD_3(HookDistributeRequestForSearch, searchWorker_->HookDistributeRequestForSearch, int, std::string, bool)

    WORKER_CONTROLLER_METHOD_2(GetSummarizationByRawKey, searchWorker_->GetSummarizationByRawKey, std::string, Summarization)
    WORKER_CONTROLLER_METHOD_2(getLabelListByDocId, searchWorker_->getLabelListByDocId, uint32_t, SearchWorker::LabelListT)
    WORKER_CONTROLLER_METHOD_2(getLabelListWithSimByDocId, searchWorker_->getLabelListWithSimByDocId, uint32_t, SearchWorker::LabelListWithSimT)
    WORKER_CONTROLLER_METHOD_1(getDistDocNum, searchWorker_->getDistDocNum, uint32_t)
    WORKER_CONTROLLER_METHOD_2(getDistKeyCount, searchWorker_->getDistKeyCount, std::string, uint32_t)

protected:
    virtual bool checkWorker(std::string& error);

private:
    boost::shared_ptr<SearchWorker> searchWorker_;
};

} // namespace sf1r

#endif // SEARCH_WORKER_CONTROLLER_H_

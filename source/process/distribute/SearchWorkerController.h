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
        ADD_WORKER_HANDLER_BEGIN(SearchWorkerController)

        ADD_WORKER_HANDLER(getDistSearchInfo)
        ADD_WORKER_HANDLER(getDistSearchResult)
        ADD_WORKER_HANDLER(getSummaryResult)
        ADD_WORKER_HANDLER(getSummaryMiningResult)
        ADD_WORKER_HANDLER(getDocumentsByIds)
        ADD_WORKER_HANDLER(getInternalDocumentId)
        ADD_WORKER_HANDLER(clickGroupLabel)
        ADD_WORKER_HANDLER(visitDoc)
        ADD_WORKER_HANDLER(getSimilarDocIdList)

        ADD_WORKER_HANDLER_END()
    }

    WORKER_CONTROLLER_METHOD_2_OUT(getDistSearchInfo, searchWorker_->getDistSearchInfo, KeywordSearchActionItem, DistKeywordSearchInfo)

    WORKER_CONTROLLER_METHOD_2_OUT(getDistSearchResult, searchWorker_->getDistSearchResult, KeywordSearchActionItem, DistKeywordSearchResult)

    WORKER_CONTROLLER_METHOD_2_OUT(getSummaryResult, searchWorker_->getSummaryResult, KeywordSearchActionItem, KeywordSearchResult)

    WORKER_CONTROLLER_METHOD_2_OUT(getSummaryMiningResult, searchWorker_->getSummaryMiningResult, KeywordSearchActionItem, KeywordSearchResult)

    WORKER_CONTROLLER_METHOD_2_OUT(getDocumentsByIds, searchWorker_->getDocumentsByIds, GetDocumentsByIdsActionItem, RawTextResultFromSIA)

    WORKER_CONTROLLER_METHOD_2_OUT(getInternalDocumentId, searchWorker_->getInternalDocumentId, izenelib::util::UString, uint64_t)

    WORKER_CONTROLLER_METHOD_1_RET(clickGroupLabel, searchWorker_->clickGroupLabel, ClickGroupLabelActionItem, bool)

    WORKER_CONTROLLER_METHOD_1_RET(visitDoc, searchWorker_->visitDoc, uint32_t, bool)

    WORKER_CONTROLLER_METHOD_3_OUT(getSimilarDocIdList, searchWorker_->getSimilarDocIdList, uint64_t, uint32_t, SimilarDocIdListType)

protected:
    virtual bool checkWorker(std::string& error);

private:
    boost::shared_ptr<SearchWorker> searchWorker_;
};

} // namespace sf1r

#endif // SEARCH_WORKER_CONTROLLER_H_

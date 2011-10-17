/**
 * @file WorkerService.h
 * @author Zhongxia Li
 * @date Jul 5, 2011
 * @brief Worker Service encapsulated service interfaces (such as Search, Mining..) of SF1.
 */
#ifndef WORKER_SERVICE_H_
#define WORKER_SERVICE_H_

#include <query-manager/ActionItem.h>
#include <query-manager/SearchKeywordOperation.h>
#include <la-manager/AnalysisInformation.h>
#include <common/ResultType.h>

#include <util/get.h>
#include <net/aggregator/JobInfo.h>
#include <ir/id_manager/IDManager.h>
#include <question-answering/QuestionAnalysis.h>

#include <boost/shared_ptr.hpp>

namespace sf1r
{
using izenelib::ir::idmanager::IDManager;

typedef std::vector<std::pair<uint64_t, float> > SimilarDocIdListType;

class IndexBundleConfiguration;
class IndexSearchService;
class MiningSearchService;
class RecommendSearchService;
class QueryLogSearchService;
class User;
class IndexManager;
class DocumentManager;
class LAManager;
class SearchManager;
class MiningManager;

class WorkerService
{
public:
    WorkerService();

public:
    /**
     * Worker services (interfaces)
     * 1) add to WorkerServer to provide remote procedure call
     * 2) add to AggregatorManager::initLocalWorkerCaller to provide in-procedure call
     * @{
     */

    /// search
    bool getDistSearchInfo(const KeywordSearchActionItem& actionItem, DistKeywordSearchInfo& resultItem);

    bool getDistSearchResult(const KeywordSearchActionItem& actionItem, DistKeywordSearchResult& resultItem);

    bool getSummaryMiningResult(const KeywordSearchActionItem& actionItem, KeywordSearchResult& resultItem);

    /// documents
    bool getDocumentsByIds(const GetDocumentsByIdsActionItem& actionItem, RawTextResultFromSIA& resultItem);

    bool getInternalDocumentId(const izenelib::util::UString& actionItem, uint64_t& resultItem);

    /// mining
    bool getSimilarDocIdList(uint64_t& documentId, uint32_t& maxNum, SimilarDocIdListType& result);

    bool clickGroupLabel(const ClickGroupLabelActionItem& actionItem, bool& ret);

    bool visitDoc(const uint32_t& docId, bool& ret);

    /** @} */

    bool doLocalSearch(const KeywordSearchActionItem& actionItem, KeywordSearchResult& resultItem);

private:
    template <typename ResultItemType>
    bool getSearchResult_(
            const KeywordSearchActionItem& actionItem,
            ResultItemType& resultItem,
            bool isDistributedSearch = true);

    bool getSummaryMiningResult_(
            const KeywordSearchActionItem& actionItem,
            KeywordSearchResult& resultItem,
            bool isDistributedSearch = true);

    void analyze_(const std::string& qstr, std::vector<izenelib::util::UString>& results);

    template <typename ResultItemT>
    bool buildQuery(
            SearchKeywordOperation& actionOperation,
            std::vector<std::vector<izenelib::util::UString> >& propertyQueryTermList,
            ResultItemT& resultItem,
            PersonalSearchInfo& personalSearchInfo);

    template<typename ActionItemT, typename ResultItemT>
    bool  getResultItem(ActionItemT& actionItem, const std::vector<sf1r::docid_t>& docsInPage,
        const vector<vector<izenelib::util::UString> >& propertyQueryTermList, ResultItemT& resultItem);

    template <typename ResultItemType>
    bool removeDuplicateDocs(
            const KeywordSearchActionItem& actionItem,
            ResultItemType& resultItem);
            
    template <typename ResultItemType>
    bool ecFilter(
            const KeywordSearchActionItem& actionItem,
            ResultItemType& resultItem);


private:
    IndexBundleConfiguration* bundleConfig_;
    RecommendSearchService* recommendSearchService_;
    QueryLogSearchService* queryLogSearchService_;

    boost::shared_ptr<LAManager> laManager_;
    boost::shared_ptr<IDManager> idManager_;
    boost::shared_ptr<DocumentManager> documentManager_;
    boost::shared_ptr<IndexManager> indexManager_;
    boost::shared_ptr<SearchManager> searchManager_;
    boost::shared_ptr<MiningManager> miningManager_;
    ilplib::qa::QuestionAnalysis* pQA_;

    AnalysisInfo analysisInfo_;

    friend class WorkerServer;
    friend class IndexSearchService;
    friend class IndexBundleActivator;
    friend class MiningBundleActivator;
    friend class ProductBundleActivator;
};

}

#endif /* WORKER_SERVICE_H_ */

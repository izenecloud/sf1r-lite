/**
 * @file WorkerService.h
 * @author Zhongxia Li
 * @date Jul 5, 2011
 * @brief Worker Service provide services of local SF1 server.
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

class IndexBundleConfiguration;
class IndexSearchService;
class MiningSearchService;
class RecommendSearchService;
class User;
class IndexManager;
class DocumentManager;
class LAManager;
class SearchManager;

class WorkerService
{
public:
    WorkerService();

public:
    /**
     * Worker services (interfaces)
     * xxx, use different function names instead of overload.
     * @{
     */

    bool getDistSearchInfo(const KeywordSearchActionItem& actionItem, DistKeywordSearchInfo& resultItem);

    bool getDistSearchResult(const KeywordSearchActionItem& actionItem, DistKeywordSearchResult& resultItem);

    bool getSummaryMiningResult(const KeywordSearchActionItem& actionItem, KeywordSearchResult& resultItem);

    bool getDocumentsByIds(const GetDocumentsByIdsActionItem& actionItem, RawTextResultFromSIA& resultItem);

    bool getInternalDocumentId(const izenelib::util::UString& actionItem, uint64_t& resultItem);

    bool clickGroupLabel(const ClickGroupLabelActionItem& actionItem, bool& ret);

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
            IndexBundleConfiguration& bundleConfig,
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


private:
    IndexBundleConfiguration* bundleConfig_;
    MiningSearchService* miningSearchService_;
    RecommendSearchService* recommendSearchService_;

    boost::shared_ptr<LAManager> laManager_;
    boost::shared_ptr<IDManager> idManager_;
    boost::shared_ptr<DocumentManager> documentManager_;
    boost::shared_ptr<IndexManager> indexManager_;
    boost::shared_ptr<SearchManager> searchManager_;
    ilplib::qa::QuestionAnalysis* pQA_;

    AnalysisInfo analysisInfo_;

    friend class IndexSearchService;
    friend class IndexBundleActivator;
};

}

#endif /* WORKER_SERVICE_H_ */

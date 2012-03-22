/**
 * @file SearchWorker.h
 * @author Zhongxia Li
 * @date Jul 5, 2011
 * @brief SearchWorker provides index and mining search interfaces.
 */
#ifndef SEARCH_WORKER_H_
#define SEARCH_WORKER_H_

#include <query-manager/ActionItem.h>
#include <query-manager/SearchKeywordOperation.h>
#include <la-manager/AnalysisInformation.h>
#include <common/ResultType.h>

#include <util/get.h>
#include <net/aggregator/Typedef.h>
#include <question-answering/QuestionAnalysis.h>

#include <boost/shared_ptr.hpp>

namespace sf1r
{
using izenelib::ir::idmanager::IDManager;

typedef std::vector<std::pair<uint64_t, float> > SimilarDocIdListType;

class IndexBundleConfiguration;
class MiningSearchService;
class RecommendSearchService;
class User;
class IndexManager;
class DocumentManager;
class LAManager;
class SearchManager;
class MiningManager;
class QueryIdentity;
class SearchCache;

class SearchWorker
{
public:
    SearchWorker(IndexBundleConfiguration* bundleConfig);

public:
    /**
     * Worker services (interfaces)
     * 1) add to WorkerServer to provide remote procedure call
     * 2) add Aggregator to provide in-procedure call
     * @{
     */

    /// search
    bool getDistSearchInfo(const KeywordSearchActionItem& actionItem, DistKeywordSearchInfo& resultItem);

    bool getDistSearchResult(const KeywordSearchActionItem& actionItem, DistKeywordSearchResult& resultItem);

    bool getSummaryResult(const KeywordSearchActionItem& actionItem, KeywordSearchResult& resultItem);

    bool getSummaryMiningResult(const KeywordSearchActionItem& actionItem, KeywordSearchResult& resultItem);

    /// documents
    bool getDocumentsByIds(const GetDocumentsByIdsActionItem& actionItem, RawTextResultFromSIA& resultItem);

    bool getInternalDocumentId(const izenelib::util::UString& actionItem, uint64_t& resultItem);

    /// mining
    bool getSimilarDocIdList(uint64_t documentId, uint32_t maxNum, SimilarDocIdListType& result);

    bool clickGroupLabel(const ClickGroupLabelActionItem& actionItem);

    bool visitDoc(const uint32_t& docId);

    /** @} */

    void makeQueryIdentity(
            QueryIdentity& identity,
            const KeywordSearchActionItem& item,
            int8_t distActionType = 0,
            uint32_t start = 0);

    bool doLocalSearch(const KeywordSearchActionItem& actionItem, KeywordSearchResult& resultItem);

    void reset_cache(
            bool rType,
            docid_t id,
            const std::map<std::string, pair<PropertyDataType, izenelib::util::UString> >& rTypeFieldValue);

    void reset_all_property_cache();

private:
    template <typename ResultItemType>
    bool getSearchResult_(
            const KeywordSearchActionItem& actionItem,
            ResultItemType& resultItem,
            bool isDistributedSearch = true);

    template <typename ResultItemType>
    bool getSearchResult_(
            const KeywordSearchActionItem& actionItem,
            ResultItemType& resultItem,
            QueryIdentity& identity,
            bool isDistributedSearch = true);

    bool getSummaryMiningResult_(
            const KeywordSearchActionItem& actionItem,
            KeywordSearchResult& resultItem,
            bool isDistributedSearch = true);

    bool getSummaryResult_(
            const KeywordSearchActionItem& actionItem,
            KeywordSearchResult& resultItem,
            bool isDistributedSearch = true);

    void analyze_(const std::string& qstr, std::vector<izenelib::util::UString>& results, bool isQA);

    template <typename ResultItemT>
    bool buildQuery(
            SearchKeywordOperation& actionOperation,
            std::vector<std::vector<izenelib::util::UString> >& propertyQueryTermList,
            ResultItemT& resultItem,
            PersonalSearchInfo& personalSearchInfo);

    template<typename ActionItemT, typename ResultItemT>
    bool  getResultItem(
            ActionItemT& actionItem,
            const std::vector<sf1r::docid_t>& docsInPage,
            const vector<vector<izenelib::util::UString> >& propertyQueryTermList,
            ResultItemT& resultItem);

    template <typename ResultItemType>
    bool removeDuplicateDocs(
            const KeywordSearchActionItem& actionItem,
            ResultItemType& resultItem);

private:
    IndexBundleConfiguration* bundleConfig_;
    RecommendSearchService* recommendSearchService_;

    boost::shared_ptr<LAManager> laManager_;
    boost::shared_ptr<IDManager> idManager_;
    boost::shared_ptr<DocumentManager> documentManager_;
    boost::shared_ptr<IndexManager> indexManager_;
    boost::shared_ptr<SearchManager> searchManager_;
    boost::shared_ptr<MiningManager> miningManager_;
    boost::shared_ptr<SearchCache> searchCache_;
    ilplib::qa::QuestionAnalysis* pQA_;

    AnalysisInfo analysisInfo_;

    friend class IndexBundleActivator;
    friend class MiningBundleActivator;
    friend class ProductBundleActivator;
    friend class LocalItemFactory;
    friend class SearchMasterItemManagerTestFixture;
};

}

#endif /* SEARCH_WORKER_H_ */

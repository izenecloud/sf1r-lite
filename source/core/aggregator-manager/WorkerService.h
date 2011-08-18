/**
 * @file WorkerService.h
 * @author Zhongxia Li
 * @date Jul 5, 2011
 * @brief Worker Service provide services of local SF1 server.
 */
#ifndef WORKER_SERVICE_H_
#define WORKER_SERVICE_H_

#include <query-manager/ActionItem.h>
#include <la-manager/AnalysisInformation.h>
#include <common/ResultType.h>

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
     * Publish services to local procedure (in-process server).
     * @{
     */

    bool call(
            const std::string& func,
            const KeywordSearchActionItem& request,
            DistKeywordSearchInfo& result,
            std::string& error)
    {
        if (func == "getDistSearchInfo")
        {
            return getDistSearchInfo(request, result);
        }

        return false;
    }

    bool call(
            const std::string& func,
            const KeywordSearchActionItem& request,
            DistKeywordSearchResult& result,
            std::string& error)
    {
        if (func == "getSearchResult")
        {
            return processGetSearchResult(request, result);
        }

        return false;
    }

    bool call(
            const std::string& func,
            const KeywordSearchActionItem& request,
            KeywordSearchResult& result,
            std::string& error)
    {
        if (func == "getSummaryResult")
        {
            return processGetSummaryResult(request, result);
        }
        else
        {
            error = "no method!";
        }

        return false;
    }

    bool call(
    		const std::string& func,
            const GetDocumentsByIdsActionItem& request,
            RawTextResultFromSIA& result,
            std::string& error)
    {
        if (func == "getDocumentsByIds")
        {
            return processGetDocumentsByIds(request, result);
        }

        error = "no method!";
        return false;
    }

    bool call(
            const std::string& func,
            const clickGroupLabelActionItem& request,
            bool& result,
            std::string& error)
    {
        if (func == "clickGroupLabel")
        {
            clickGroupLabel(request, result);
            return result;
        }

        error = "no method!";
        return false;
    }

    /** @}*/

public:
    /**
     * Services (interfaces) for publication
     * @{
     */

    bool getDistSearchInfo(const KeywordSearchActionItem& actionItem, DistKeywordSearchInfo& resultItem);

    bool processGetSearchResult(const KeywordSearchActionItem& actionItem, DistKeywordSearchResult& resultItem);

    bool processGetSummaryResult(const KeywordSearchActionItem& actionItem, KeywordSearchResult& resultItem);

    bool processGetDocumentsByIds(const GetDocumentsByIdsActionItem& actionItem, RawTextResultFromSIA& resultItem);

    bool clickGroupLabel(const clickGroupLabelActionItem& actionItem, bool& ret);

    /** @} */

private:
    template <typename ResultItemType>
    bool getSearchResult(
            KeywordSearchActionItem& actionItem,
            ResultItemType& resultItem,
            bool isDistributedSearch = true);

    bool getSummaryMiningResult(
            KeywordSearchActionItem& actionItem,
            KeywordSearchResult& resultItem,
            bool isDistributedSearch = true);

    void analyze_(const std::string& qstr, std::vector<izenelib::util::UString>& results);

    template<typename ActionItemT, typename ResultItemT>
    bool  getResultItem(ActionItemT& actionItem, const std::vector<sf1r::docid_t>& docsInPage,
        const vector<vector<izenelib::util::UString> >& propertyQueryTermList, ResultItemT& resultItem);

    template <typename ResultItemType>
    bool removeDuplicateDocs(
            KeywordSearchActionItem& actionItem,
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

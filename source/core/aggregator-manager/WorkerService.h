/**
 * @file WorkerService.h
 * @author Zhongxia Li
 * @date Jul 5, 2011
 * @brief Worker Service provides services of local SF1 server.
 * It can work as a RPC Server by calling startServer(host,port,threadnum), which provides remote services for Aggregator(client).
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
     * interfaces for handling local request (call directly)
     * @{
     */

    bool call(
            const std::string& func,
            const KeywordSearchActionItem& request,
            KeywordSearchResult& result,
            std::string& error)
    {
        if (func == "getSearchResult")
        {
            return processGetSearchResult(request, result);
        }
        else if (func == "getSummaryResult")
        {
            return processGetSummaryResult(request, result);
        }
        else
        {
            error = "no method!";
        }

        return false;
    }

    /** @}*/

public:
    /**
     * Services for publication
     * @{
     */

    bool processGetSearchResult(const KeywordSearchActionItem& actionItem, KeywordSearchResult& resultItem);

    bool processGetSummaryResult(const KeywordSearchActionItem& actionItem, KeywordSearchResult& resultItem);

    /** @} */

private:
    bool getSearchResult(
            KeywordSearchActionItem& actionItem,
            KeywordSearchResult& resultItem,
            std::vector<std::vector<izenelib::util::UString> >& propertyQueryTermList);

    bool getSummaryMiningResult(
            KeywordSearchActionItem& actionItem,
            KeywordSearchResult& resultItem,
            std::vector<std::vector<izenelib::util::UString> >& propertyQueryTermList);

    void analyze_(const std::string& qstr, std::vector<izenelib::util::UString>& results);

    template<typename ActionItemT, typename ResultItemT>
    bool  getResultItem(ActionItemT& actionItem, const std::vector<sf1r::docid_t>& docsInPage,
        const vector<vector<izenelib::util::UString> >& propertyQueryTermList, ResultItemT& resultItem);

    bool removeDuplicateDocs(
            KeywordSearchActionItem& actionItem,
            KeywordSearchResult& resultItem);


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

    friend class IndexBundleActivator;
};

}

#endif /* WORKER_SERVICE_H_ */

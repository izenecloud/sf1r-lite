/**
 * @file SearchThreadMaster.h
 * @brief the master to manage multiple search threads.
 * @author Vincent Lee, Jun Jiang
 * @date Created 2013-01-04
 */

#ifndef SF1R_SEARCH_THREAD_MASTER_H
#define SF1R_SEARCH_THREAD_MASTER_H

#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/threadpool.hpp>

namespace sf1r
{
class IndexBundleConfiguration;
class SearchKeywordOperation;
class DistKeywordSearchInfo;
class KeywordSearchResult;
class DocumentManager;
class SearchManagerPreProcessor;
class SearchThreadWorker;
struct SearchThreadParam;

class SearchThreadMaster
{
public:
    SearchThreadMaster(
        const IndexBundleConfiguration& config,
        SearchManagerPreProcessor& preprocessor,
        const boost::shared_ptr<DocumentManager>& documentManager,
        SearchThreadWorker& searchThreadWorker);

    void prepareThreadParams(
        const SearchKeywordOperation& actionOperation,
        DistKeywordSearchInfo& distSearchInfo,
        std::size_t heapSize,
        std::vector<SearchThreadParam>& threadParams);

    bool runThreadParams(
        std::vector<SearchThreadParam>& threadParams);

    bool mergeThreadParams(
        std::vector<SearchThreadParam>& threadParams) const;

    bool fetchSearchResult(
        std::size_t offset,
        SearchThreadParam& threadParam,
        KeywordSearchResult& searchResult);

private:
    void getThreadInfo_(
        const DistKeywordSearchInfo& distSearchInfo,
        std::size_t& threadNum,
        std::size_t& runningNode);

    bool runSingleThread_(
        SearchThreadParam& threadParam);

    bool runMultiThreads_(
        std::vector<SearchThreadParam>& threadParams);

    void runSearchJob_(
        SearchThreadParam* pParam,
        boost::detail::atomic_count* finishedJobs);

private:
    const bool isParallelEnabled_;

    boost::threadpool::pool threadpool_;

    SearchManagerPreProcessor& preprocessor_;

    boost::shared_ptr<DocumentManager> documentManagerPtr_;

    SearchThreadWorker& searchThreadWorker_;
};

} // namespace sf1r

#endif // SF1R_SEARCH_THREAD_MASTER_H

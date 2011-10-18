#ifndef INDEX_BUNDLE_SEARCH_SERVICE_H
#define INDEX_BUNDLE_SEARCH_SERVICE_H

#include "IndexBundleConfiguration.h"

#include <common/sf1_serialization_types.h>
#include <common/type_defs.h>
#include <common/Status.h>

#include <query-manager/SearchKeywordOperation.h>
#include <query-manager/ActionItem.h>
#include <document-manager/Document.h>
#include <aggregator-manager/WorkerService.h>
#include <configuration-manager/BrokerAgentConfig.h>

#include <ir/id_manager/IDManager.h>
#include <question-answering/QuestionAnalysis.h>

#include <util/osgi/IService.h>

#include <boost/shared_ptr.hpp>


namespace sf1r
{
using izenelib::ir::idmanager::IDManager;
using namespace net::aggregator;

class AggregatorManager;
class SearchCache;
class IndexSearchService : public ::izenelib::osgi::IService
{
public:
    IndexSearchService(IndexBundleConfiguration* config);

    ~IndexSearchService();

    boost::shared_ptr<AggregatorManager> getAggregatorManager();

    const IndexBundleConfiguration* getBundleConfig();

    void OnUpdateSearchCache();

public:
    bool getSearchResult(KeywordSearchActionItem& actionItem, KeywordSearchResult& resultItem);

    bool getDocumentsByIds(const GetDocumentsByIdsActionItem& actionItem, RawTextResultFromSIA& resultItem);

    bool getInternalDocumentId(const std::string& collectionName, const izenelib::util::UString& scdDocumentId, uint64_t& internalId);

private:
    IndexBundleConfiguration* bundleConfig_;
    boost::shared_ptr<AggregatorManager> aggregatorManager_;
    boost::shared_ptr<WorkerService> workerService_;

    boost::scoped_ptr<SearchCache> cache_; // for Master Node

    friend class WorkerServer;
    friend class IndexBundleActivator;
    friend class MiningBundleActivator;
    friend class ProductBundleActivator;
};

}

#endif


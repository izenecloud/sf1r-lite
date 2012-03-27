#ifndef INDEX_BUNDLE_SEARCH_SERVICE_H
#define INDEX_BUNDLE_SEARCH_SERVICE_H

#include "IndexBundleConfiguration.h"

#include <common/sf1_serialization_types.h>
#include <common/type_defs.h>
#include <common/Status.h>

#include <query-manager/SearchKeywordOperation.h>
#include <query-manager/ActionItem.h>

#include <util/osgi/IService.h>

#include <boost/shared_ptr.hpp>

namespace sf1r
{
using namespace net::aggregator;

class SearchAggregator;
class SearchCache;
class SearchWorker;
class IndexSearchService : public ::izenelib::osgi::IService
{
public:
    IndexSearchService(IndexBundleConfiguration* config);

    ~IndexSearchService();

    boost::shared_ptr<SearchAggregator> getSearchAggregator();

    const IndexBundleConfiguration* getBundleConfig();

    void OnUpdateSearchCache();

public:
    bool getSearchResult(KeywordSearchActionItem& actionItem, KeywordSearchResult& resultItem);

    bool getDocumentsByIds(const GetDocumentsByIdsActionItem& actionItem, RawTextResultFromSIA& resultItem);

    bool getInternalDocumentId(const std::string& collectionName, const izenelib::util::UString& scdDocumentId, uint64_t& internalId);

private:
    IndexBundleConfiguration* bundleConfig_;
    boost::shared_ptr<SearchAggregator> searchAggregator_;
    boost::shared_ptr<SearchWorker> searchWorker_;

    boost::scoped_ptr<SearchCache> searchCache_; // for Master Node

    friend class SearchWorkerController;
    friend class IndexBundleActivator;
    friend class MiningBundleActivator;
    friend class ProductBundleActivator;
    friend class LocalItemFactory;
    friend class SearchMasterItemManagerTestFixture;
};

}

#endif

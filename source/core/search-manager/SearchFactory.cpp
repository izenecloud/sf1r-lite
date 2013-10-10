#include "SearchFactory.h"
#include "NormalSearch.h"
#include "ZambeziSearch.h"
#include <bundles/index/IndexBundleConfiguration.h>

using namespace sf1r;

SearchFactory::SearchFactory(
    const IndexBundleConfiguration& config,
    const boost::shared_ptr<DocumentManager>& documentManager,
    const boost::shared_ptr<InvertedIndexManager>& indexManager,
    const boost::shared_ptr<RankingManager>& rankingManager)
    : config_(config)
    , documentManager_(documentManager)
    , indexManager_(indexManager)
    , rankingManager_(rankingManager)
{
}

QueryBuilder* SearchFactory::createQueryBuilder(
    const QueryBuilder::schema_map& schemaMap) const
{
    return new QueryBuilder(documentManager_,
                            indexManager_,
                            schemaMap,
                            config_.filterCacheNum_);
}

SearchBase* SearchFactory::createSearchBase(
    SearchingMode::SearchingModeType mode,
    SearchManagerPreProcessor& preprocessor,
    QueryBuilder& queryBuilder) const
{
    switch (mode)
    {
    case SearchingMode::DefaultSearchingMode:
        return new NormalSearch(config_,
                                documentManager_,
                                indexManager_,
                                rankingManager_,
                                preprocessor,
                                queryBuilder);

    case SearchingMode::ZAMBEZI:
        return new ZambeziSearch(*documentManager_,
                                 preprocessor,
                                 queryBuilder);

    default:
        return NULL;
    }
}

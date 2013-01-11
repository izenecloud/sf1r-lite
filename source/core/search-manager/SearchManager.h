#ifndef CORE_SEARCH_MANAGER_SEARCH_MANAGER_H
#define CORE_SEARCH_MANAGER_SEARCH_MANAGER_H

#include "SearchManagerPreProcessor.h"
#include "TopKReranker.h"
#include "FuzzySearchRanker.h"
#include "QueryBuilder.h"
#include "SearchBase.h"
#include <ir/id_manager/IDManager.h>

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

using izenelib::ir::idmanager::IDManager;

namespace sf1r
{
class IndexBundleConfiguration;
class DocumentManager;
class IndexManager;
class RankingManager;
class MiningManager;

class SearchManager
{
public:
    SearchManager(
        const IndexBundleConfiguration& config,
        const boost::shared_ptr<IDManager>& idManager,
        const boost::shared_ptr<DocumentManager>& documentManager,
        const boost::shared_ptr<IndexManager>& indexManager,
        const boost::shared_ptr<RankingManager>& rankingManager);

    void setMiningManager(
        const boost::shared_ptr<MiningManager>& miningManager);

private:
    SearchManagerPreProcessor preprocessor_;

public:
    TopKReranker topKReranker_;

    FuzzySearchRanker fuzzySearchRanker_;

    boost::scoped_ptr<QueryBuilder> queryBuilder_;

    boost::scoped_ptr<SearchBase> searchBase_;
};

} // end - namespace sf1r

#endif // CORE_SEARCH_MANAGER_SEARCH_MANAGER_H

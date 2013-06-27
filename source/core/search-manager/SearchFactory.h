/**
 * @file SearchFactory.h
 * @brief create the instance of QueryBuilder and SearchBase.
 * @author Jun Jiang
 * @date Created 2013-01-11
 */

#ifndef SF1R_SEARCH_FACTORY_H
#define SF1R_SEARCH_FACTORY_H

#include "QueryBuilder.h"

#include <boost/shared_ptr.hpp>

namespace sf1r
{
class IndexBundleConfiguration;
class DocumentManager;
class InvertedIndexManager;
class RankingManager;
class SearchBase;
class SearchManagerPreProcessor;

class SearchFactory
{
public:
    SearchFactory(
        const IndexBundleConfiguration& config,
        const boost::shared_ptr<DocumentManager>& documentManager,
        const boost::shared_ptr<InvertedIndexManager>& indexManager,
        const boost::shared_ptr<RankingManager>& rankingManager);

    QueryBuilder* createQueryBuilder(
        const QueryBuilder::schema_map& schemaMap) const;

    SearchBase* createSearchBase(
        SearchManagerPreProcessor& preprocessor,
        QueryBuilder& queryBuilder) const;

private:
    const IndexBundleConfiguration& config_;

    const boost::shared_ptr<DocumentManager>& documentManager_;

    const boost::shared_ptr<InvertedIndexManager>& indexManager_;

    const boost::shared_ptr<RankingManager>& rankingManager_;
};

} // namespace sf1r

#endif // SF1R_SEARCH_FACTORY_H

/**
 * @file ZambeziSearch.h
 * @brief get topk docs from zambezi index, it also does the work such as
 *        filter, group, sort, etc.
 */

#ifndef SF1R_ZAMBEZI_SEARCH_H
#define SF1R_ZAMBEZI_SEARCH_H

#include "SearchBase.h"
#include <common/inttypes.h>
#include <mining-manager/product-scorer/ProductScorer.h>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <string>
#include <vector>

namespace sf1r
{
class DocumentManager;
class SearchManagerPreProcessor;
class QueryBuilder;
class ZambeziManager;

namespace faceted
{
class GroupFilterBuilder;
}

class ZambeziSearch : public SearchBase
{
public:
    ZambeziSearch(
        DocumentManager& documentManager,
        SearchManagerPreProcessor& preprocessor,
        QueryBuilder& queryBuilder);

    virtual void setMiningManager(
        const boost::shared_ptr<MiningManager>& miningManager);

    virtual bool search(
        const SearchKeywordOperation& actionOperation,
        KeywordSearchResult& searchResult,
        std::size_t limit,
        std::size_t offset);

private:
    bool getCandidateDocs_(
        const std::string& query,
        std::vector<docid_t>& candidates,
        std::vector<float>& scores);

    void rankTopKDocs_(
        const std::vector<docid_t>& candidates,
        const std::vector<float>& scores,
        const SearchKeywordOperation& actionOperation,
        std::size_t limit,
        std::size_t offset,
        KeywordSearchResult& searchResult);

private:
    DocumentManager& documentManager_;

    SearchManagerPreProcessor& preprocessor_;

    QueryBuilder& queryBuilder_;

    const faceted::GroupFilterBuilder* groupFilterBuilder_;

    ZambeziManager* zambeziManager_;

    boost::scoped_ptr<ProductScorer> productScorer_;
};

} // namespace sf1r

#endif // SF1R_ZAMBEZI_SEARCH_H

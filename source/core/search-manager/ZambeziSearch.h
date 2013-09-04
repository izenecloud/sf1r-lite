/**
 * @file ZambeziSearch.h
 * @brief get topk docs from zambezi index, it also does the work such as
 *        filter, group, sort, etc.
 */

#ifndef SF1R_ZAMBEZI_SEARCH_H
#define SF1R_ZAMBEZI_SEARCH_H

#include "SearchBase.h"
#include <common/inttypes.h>
#include <string>
#include <vector>

namespace sf1r
{
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
    bool getTopKDocs_(
        const std::string& query,
        std::vector<docid_t>& docIds,
        std::vector<float>& scores);

private:
    SearchManagerPreProcessor& preprocessor_;

    QueryBuilder& queryBuilder_;

    boost::shared_ptr<MiningManager> miningManager_;

    const faceted::GroupFilterBuilder* groupFilterBuilder_;

    ZambeziManager* zambeziManager_;
};

} // namespace sf1r

#endif // SF1R_ZAMBEZI_SEARCH_H

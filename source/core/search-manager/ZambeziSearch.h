/**
 * @file ZambeziSearch.h
 * @brief get topk docs from zambezi index, it also does the work such as
 *        filter, group, sort, etc.
 */

#ifndef SF1R_ZAMBEZI_SEARCH_H
#define SF1R_ZAMBEZI_SEARCH_H

#include "SearchBase.h"
#include <common/inttypes.h>
#include <mining-manager/group-manager/GroupParam.h>
#include <boost/shared_ptr.hpp>
#include <string>
#include <vector>

namespace sf1r
{
class DocumentManager;
class SearchManagerPreProcessor;
class QueryBuilder;
class ZambeziManager;
class PropSharedLockSet;

namespace faceted
{
class GroupFilterBuilder;
class PropValueTable;
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
    void getTopLabels_(
        const std::vector<unsigned int>& docIdList,
        PropSharedLockSet& propSharedLockSet,
        faceted::GroupParam::GroupLabelMap& topLabelMap);

private:
    DocumentManager& documentManager_;

    SearchManagerPreProcessor& preprocessor_;

    QueryBuilder& queryBuilder_;

    const faceted::GroupFilterBuilder* groupFilterBuilder_;

    ZambeziManager* zambeziManager_;

    const faceted::PropValueTable* categoryValueTable_;
};

} // namespace sf1r

#endif // SF1R_ZAMBEZI_SEARCH_H

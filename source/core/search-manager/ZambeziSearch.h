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
#include <mining-manager/group-manager/PropValueTable.h> // pvid_t
#include <mining-manager/group-manager/GroupManager.h>
#include <mining-manager/attr-manager/AttrManager.h>
#include <ir/Zambezi/Consts.hpp>
#include <mining-manager/attr-manager/AttrTable.h>
#include <index-manager/zambezi-tokenizer/ZambeziTokenizer.h>
#include <search-manager/NumericPropertyTableBuilder.h>
#include <util/ustring/UString.h>
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
class ProductScorer;
class HitQueue;
class ScoreDoc;

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
        QueryBuilder& queryBuilder,
        ZambeziManager* zambeziManager);

    virtual void setMiningManager(
        const boost::shared_ptr<MiningManager>& miningManager);

    virtual bool search(
        const SearchKeywordOperation& actionOperation,
        KeywordSearchResult& searchResult,
        std::size_t limit,
        std::size_t offset);

private:
    void normalizeScore_(
        std::vector<docid_t>& docids,
        std::vector<float>& scores,
        std::vector<float>& productScores,
        PropSharedLockSet &sharedLockSet);
    
    void normalizeTopDocs_(
        const boost::scoped_ptr<ProductScorer>& productScorer, 
        boost::scoped_ptr<HitQueue>& scoreItemQueue,
        std::vector<ScoreDoc>& resultList, 
        PropSharedLockSet &sharedLockSet);

    void getTopLabels_(
        const std::vector<unsigned int>& docIdList,
        const std::vector<float>& rankScoreList,
        PropSharedLockSet& propSharedLockSet,
        faceted::GroupParam::GroupLabelScoreMap& topLabelMap);

    void getAnalyzedQuery_(
        const std::string& rawQuery,
        izenelib::util::UString& analyzedQuery);

    bool getZambeziAlgorithm(
         const int &algorithm,
         izenelib::ir::Zambezi::Algorithm& Algorithm);

private:
    DocumentManager& documentManager_;

    SearchManagerPreProcessor& preprocessor_;

    QueryBuilder& queryBuilder_;

    const faceted::GroupFilterBuilder* groupFilterBuilder_;

    ZambeziManager* zambeziManager_;

    const faceted::PropValueTable* categoryValueTable_;

    faceted::AttrManager* attrManager_;

    NumericPropertyTableBuilder* numericTableBuilder_;
};

} // namespace sf1r

#endif // SF1R_ZAMBEZI_SEARCH_H

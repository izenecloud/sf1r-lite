#ifndef CORE_SEARCH_MANAGER_SEARCH_MANAGER_PPEPROCESSOR_H
#define CORE_SEARCH_MANAGER_SEARCH_MANAGER_PREPROCESSOR_H

#include <configuration-manager/PropertyConfig.h>
#include <query-manager/SearchKeywordOperation.h>
#include <query-manager/ActionItem.h>
#include <common/ResultType.h>
#include <common/PropertyTermInfo.h>
#include <common/NumericPropertyTableBase.h>
#include <types.h>

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <algorithm>
#include <vector>

namespace sf1r
{

class Sorter;
class NumericPropertyTableBase;
class DocumentIterator;
class RankQueryProperty;
class PropertyRanker;
class ProductScorerFactory;
class ProductScorer;
class PropSharedLockSet;
class NumericPropertyTableBuilder;

class SearchManagerPreProcessor
{
    friend class SearchManager;
private:
    DISALLOW_COPY_AND_ASSIGN(SearchManagerPreProcessor);
    SearchManagerPreProcessor();
    ~SearchManagerPreProcessor();

    boost::unordered_map<std::string, PropertyConfig> schemaMap_;
    ProductScorerFactory* productScorerFactory_;
    NumericPropertyTableBuilder* numericTableBuilder_;

    bool getPropertyTypeByName_(
        const std::string& name,
        PropertyDataType& type) const;

    void prepare_sorter_customranker_(
        const SearchKeywordOperation& actionOperation,
        CustomRankerPtr& customRanker,
        boost::shared_ptr<Sorter>& pSorter);

    /**
     * rebuild custom ranker.
     * @param actionItem
     * @return
     */
    CustomRankerPtr buildCustomRanker_(KeywordSearchActionItem& actionItem);

    /**
     * @brief get data list of each sort property for documents referred by docIdList,
     * used in distributed search for merging topk results.
     * @param pSorter [OUT]
     * @param docIdList [IN]
     * @param distSearchInfo [OUT]
     */
    void fillSearchInfoWithSortPropertyData_(
        Sorter* pSorter,
        std::vector<unsigned int>& docIdList,
        DistKeywordSearchInfo& distSearchInfo);

    template<class UnaryOperator>
    void PreparePropertyList(
        std::vector<std::string>& indexPropertyList,
        std::vector<propertyid_t>& indexPropertyIdList,
        UnaryOperator op)
    {
#if PREFETCH_TERMID
        std::stable_sort (indexPropertyList.begin(), indexPropertyList.end());
#endif
        std::transform(
            indexPropertyList.begin(),
            indexPropertyList.end(),
            indexPropertyIdList.begin(),
            op);

    }

    void PreparePropertyTermIndex(
        const std::map<std::string, PropertyTermInfo>& propertyTermInfoMap,
        const std::vector<std::string>& indexPropertyList,
        std::vector<std::map<termid_t, unsigned> >& termIndexMaps);

    ProductScorer* createProductScorer(
        const KeywordSearchActionItem& actionItem,
        PropSharedLockSet& propSharedLockSet,
        ProductScorer* relevanceScorer);

    bool isProductRanking(const KeywordSearchActionItem& actionItem) const;

    bool isNeedCustomDocIterator(const KeywordSearchActionItem& actionItem) const;

    bool isNeedRerank(const KeywordSearchActionItem& actionItem) const;

public:
    /**
     * @return true when @p sortPriorityList_ contains "_rank" property.
     */
    static bool hasSortByRankProp(
        const KeywordSearchActionItem::SortPriorityList& sortPriorityList);

    /**
     * @return true when @p sortPriorityList_ has only "_rank" property.
     */
    static bool isSortByRankProp(
        const KeywordSearchActionItem::SortPriorityList& sortPriorityList);
};

} // end of sf1r
#endif

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
class DocumentManager;
class RankQueryProperty;
class PropertyRanker;
class ProductScorerFactory;
class ProductScorer;
class PropSharedLockSet;
class NumericPropertyTableBuilder;
class RTypeStringPropTableBuilder;

class SearchManagerPreProcessor
{
public:
    SearchManagerPreProcessor(const IndexBundleSchema& indexSchema);

    typedef boost::unordered_map<std::string, PropertyConfig> SchemaMap;
    const SchemaMap& getSchemaMap() const { return schemaMap_; }

    void setProductScorerFactory(ProductScorerFactory* factory)
    {
        productScorerFactory_ = factory;
    }

    void setNumericTableBuilder(NumericPropertyTableBuilder* builder)
    {
        numericTableBuilder_ = builder;
    }
    
    void setRTypeStringPropTableBuilder(RTypeStringPropTableBuilder* builder)
    {
        rtypeStringPropTableBuilder_ = builder;
    }

    /**
     * @brief get data list of each sort property for documents referred by docIdList,
     * used in distributed search for merging topk results.
     * @param pSorter [OUT]
     * @param docIdList [IN]
     * @param distSearchInfo [OUT]
     */
    void fillSearchInfoWithSortPropertyData(
        Sorter* pSorter,
        std::vector<unsigned int>& docIdList,
        DistKeywordSearchInfo& distSearchInfo);

    void prepareSorterCustomRanker(
        const SearchKeywordOperation& actionOperation,
        boost::shared_ptr<Sorter>& pSorter,
        CustomRankerPtr& customRanker);
    
    template<class UnaryOperator>
    void preparePropertyList(
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

    void preparePropertyTermIndex(
        const std::map<std::string, PropertyTermInfo>& propertyTermInfoMap,
        const std::vector<std::string>& indexPropertyList,
        std::vector<std::map<termid_t, unsigned> >& termIndexMaps);

    ProductScorer* createProductScorer(
        const KeywordSearchActionItem& actionItem,
        PropSharedLockSet& propSharedLockSet,
        ProductScorer* relevanceScorer);

    bool isNeedCustomDocIterator(const KeywordSearchActionItem& actionItem) const;

    bool isNeedRerank(const KeywordSearchActionItem& actionItem) const;

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

private:
    DISALLOW_COPY_AND_ASSIGN(SearchManagerPreProcessor);

    bool getPropertyTypeByName_(
        const std::string& name,
        PropertyDataType& type) const;

    /**
     * rebuild custom ranker.
     * @param actionItem
     * @return
     */
    CustomRankerPtr buildCustomRanker_(KeywordSearchActionItem& actionItem);

    bool isProductRanking_(const KeywordSearchActionItem& actionItem) const;

private:
    SchemaMap schemaMap_;

    ProductScorerFactory* productScorerFactory_;

    NumericPropertyTableBuilder* numericTableBuilder_;
    
    RTypeStringPropTableBuilder* rtypeStringPropTableBuilder_;
};

} // end of sf1r
#endif

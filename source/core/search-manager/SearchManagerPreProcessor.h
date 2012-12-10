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
#include <boost/weak_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <algorithm>
#include <vector>

namespace sf1r
{

class SortPropertyCache;
class Sorter;
class NumericPropertyTableBase;
class MiningManager;
class DocumentIterator;
class RankQueryProperty;
class PropertyRanker;
class ProductScorerFactory;
class ProductScorer;

namespace faceted
{
class PropSharedLockSet;
}

class SearchManagerPreProcessor
{
    friend class SearchManager;
private:
    DISALLOW_COPY_AND_ASSIGN(SearchManagerPreProcessor);
    SearchManagerPreProcessor();
    ~SearchManagerPreProcessor();

    boost::unordered_map<std::string, PropertyConfig> schemaMap_;
    ProductScorerFactory* productScorerFactory_;

    boost::shared_ptr<NumericPropertyTableBase>& createPropertyTable(
        const std::string& propertyName,
        SortPropertyCache* pSorterCache);

    bool getPropertyTypeByName_(
        const std::string& name,
        PropertyDataType& type) const;

    void prepare_sorter_customranker_(
        const SearchKeywordOperation& actionOperation,
        CustomRankerPtr& customRanker,
        boost::shared_ptr<Sorter> &pSorter,
        SortPropertyCache* pSorterCache,
        boost::weak_ptr<MiningManager> miningManagerPtr);

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
        DistKeywordSearchInfo& distSearchInfo,
        SortPropertyCache* pSorterCache);

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
        boost::shared_ptr<Sorter> sorter,
        DocumentIterator* scoreDocIterator,
        const std::vector<RankQueryProperty>& rankQueryProps,
        const std::vector<boost::shared_ptr<PropertyRanker> >& propRankers,
        faceted::PropSharedLockSet& propSharedLockSet);

    ProductScorer* createProductScorer(
        const KeywordSearchActionItem& actionItem,
        boost::shared_ptr<Sorter> sorter,
        faceted::PropSharedLockSet& propSharedLockSet);

    bool isProductRanking(const KeywordSearchActionItem& actionItem) const;
};

} // end of sf1r
#endif

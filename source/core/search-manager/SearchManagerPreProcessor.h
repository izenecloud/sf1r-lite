#ifndef CORE_SEARCH_MANAGER_SEARCH_MANAGER_PPEPROCESSOR_H
#define CORE_SEARCH_MANAGER_SEARCH_MANAGER_PREPROCESSOR_H

#include "PropertyData.h"
#include <configuration-manager/PropertyConfig.h>
#include <query-manager/SearchKeywordOperation.h>
#include <query-manager/ActionItem.h>
#include <common/ResultType.h>
#include <common/PropertyTermInfo.h>

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <algorithm>

namespace sf1r
{

class PropertyRanker;
class ProductRankerFactory;
class SortPropertyCache;
class Sorter;
class NumericPropertyTable;
class MiningManager;

class SearchManagerPreProcessor
{
    friend class SearchManager;
private:
    SearchManagerPreProcessor();
    ~SearchManagerPreProcessor();

    boost::unordered_map<std::string, PropertyConfig> schemaMap_;

    NumericPropertyTable* createPropertyTable(const std::string& propertyName, SortPropertyCache* psortercache);

    bool getPropertyTypeByName_(
            const std::string& name,
            PropertyDataType& type) const;

    boost::shared_ptr<PropertyData> getPropertyData_(const std::string& name, SortPropertyCache* psortercache);

    void prepare_sorter_customranker_(
            const SearchKeywordOperation& actionOperation,
            CustomRankerPtr& customRanker,
            boost::shared_ptr<Sorter> &pSorter,
            SortPropertyCache* psortercache,
            boost::weak_ptr<MiningManager> miningManagerPtr
            );

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
            SortPropertyCache* psortercache);

    template<class UnaryOperator> 
        void PreparePropertyList(
            std::vector<std::string>& indexPropertyList,
            std::vector<propertyid_t>& indexPropertyIdList, 
            UnaryOperator op 
            )
        {
#if PREFETCH_TERMID
            std::stable_sort (indexPropertyList.begin(), indexPropertyList.end());
#endif
            std::transform(
                indexPropertyList.begin(),
                indexPropertyList.end(),
                indexPropertyIdList.begin(),
                op );

        }

    void PreparePropertyTermIndex(
        const std::map<std::string, PropertyTermInfo>& propertyTermInfoMap,
        const std::vector<std::string>& indexPropertyList, 
        std::vector<std::map<termid_t, unsigned> >& termIndexMaps);

};

} // end of sf1r
#endif

#include "SearchManagerPreProcessor.h"
#include "Sorter.h"
#include "NumericPropertyTableBuilder.h"
#include "DocumentIterator.h"
#include <ranking-manager/RankQueryProperty.h>
#include <ranking-manager/PropertyRanker.h>
#include <mining-manager/product-scorer/ProductScorerFactory.h>
#include <mining-manager/faceted-submanager/ctr_manager.h>
#include <mining-manager/MiningManager.h>
#include <common/SFLogger.h>
#include <util/get.h>
#include <util/ClockTimer.h>
#include <fstream>
#include <algorithm> // to_lower

using namespace sf1r;

const std::string RANK_PROPERTY("_rank");
const std::string DATE_PROPERTY("date");
const std::string CUSTOM_RANK_PROPERTY("custom_rank");

SearchManagerPreProcessor::SearchManagerPreProcessor()
    : schemaMap_()
    , productScorerFactory_(NULL)
{
}

SearchManagerPreProcessor::~SearchManagerPreProcessor()
{
}

boost::shared_ptr<NumericPropertyTableBase>&
SearchManagerPreProcessor::createPropertyTable(const std::string& propertyName, SortPropertyCache* pSorterCache)
{
    static boost::shared_ptr<NumericPropertyTableBase> emptyNumericPropertyTable;
    PropertyDataType type = UNKNOWN_DATA_PROPERTY_TYPE;

    if (getPropertyTypeByName_(propertyName, type))
    {
        return pSorterCache->getSortPropertyData(propertyName, type);
    }

    return emptyNumericPropertyTable;
}

void SearchManagerPreProcessor::prepare_sorter_customranker_(
    const SearchKeywordOperation& actionOperation,
    CustomRankerPtr& customRanker,
    boost::shared_ptr<Sorter> &pSorter,
    SortPropertyCache* pSorterCache,
    boost::weak_ptr<MiningManager> miningManagerPtr)
{
    std::vector<std::pair<std::string, bool> >& sortPropertyList
    = actionOperation.actionItem_.sortPriorityList_;
    if (!sortPropertyList.empty())
    {
        std::vector<std::pair<std::string, bool> >::iterator iter = sortPropertyList.begin();
        for (; iter != sortPropertyList.end(); ++iter)
        {
            std::string fieldNameL = iter->first;
            boost::to_lower(fieldNameL);
            // sort by custom ranking
            if (fieldNameL == CUSTOM_RANK_PROPERTY)
            {
                // prepare custom ranker data, custom score will be evaluated later as rank score
                customRanker = actionOperation.actionItem_.customRanker_;
                if (!customRanker)
                    customRanker = buildCustomRanker_(actionOperation.actionItem_);
                if (!customRanker->setPropertyData(pSorterCache))
                {
                    LOG(ERROR) << customRanker->getErrorInfo() ;
                    continue;
                }
                //customRanker->printESTree();

                if (!pSorter) pSorter.reset(new Sorter(pSorterCache));
                SortProperty* pSortProperty = new SortProperty(
                    "CUSTOM_RANK",
                    CUSTOM_RANKING_PROPERTY_TYPE,
                    SortProperty::CUSTOM,
                    iter->second);
                pSorter->addSortProperty(pSortProperty);
                continue;
            }
            // sort by rank
            if (fieldNameL == RANK_PROPERTY)
            {
                if (!pSorter) pSorter.reset(new Sorter(pSorterCache));
                SortProperty* pSortProperty = new SortProperty(
                    "RANK",
                    UNKNOWN_DATA_PROPERTY_TYPE,
                    SortProperty::SCORE,
                    iter->second);
                pSorter->addSortProperty(pSortProperty);
                continue;
            }
            // sort by date
            if (fieldNameL == DATE_PROPERTY)
            {
                if (!pSorter) pSorter.reset(new Sorter(pSorterCache));
                SortProperty* pSortProperty = new SortProperty(
                    iter->first,
                    INT64_PROPERTY_TYPE,
                    iter->second);
                pSorter->addSortProperty(pSortProperty);
                continue;
            }
            // sort by ctr (click through rate)
            if (fieldNameL == faceted::CTRManager::kCtrPropName)
            {
                if (miningManagerPtr.expired())
                {
                    DLOG(ERROR)<<"Skipped CTR sort property: Mining Manager was not initialized";
                    continue;
                }
                boost::shared_ptr<MiningManager> resourceMiningManagerPtr = miningManagerPtr.lock();
                boost::shared_ptr<faceted::CTRManager>& ctrManangerPtr
                = resourceMiningManagerPtr->GetCtrManager();
                if (!ctrManangerPtr)
                {
                    DLOG(ERROR)<<"Skipped CTR sort property: CTR Manager was not initialized";
                    continue;
                }

                pSorterCache->setCtrManager(ctrManangerPtr.get());
                if (!pSorter) pSorter.reset(new Sorter(pSorterCache));

                SortProperty* pSortProperty = new SortProperty(
                    iter->first,
                    INT32_PROPERTY_TYPE,
                    SortProperty::CTR,
                    iter->second);
                pSorter->addSortProperty(pSortProperty);
                continue;
            }

            // sort by arbitrary property
            boost::unordered_map<std::string, PropertyConfig>::iterator it
            = schemaMap_.find(iter->first);
            if (it == schemaMap_.end())
                continue;

            PropertyConfig& propertyConfig = it->second;
            if (!propertyConfig.isIndex() || propertyConfig.isAnalyzed())
                continue;

            PropertyDataType propertyType = propertyConfig.getType();
            switch (propertyType)
            {
            case STRING_PROPERTY_TYPE:
            case INT32_PROPERTY_TYPE:
            case FLOAT_PROPERTY_TYPE:
            case DATETIME_PROPERTY_TYPE:
            case INT8_PROPERTY_TYPE:
            case INT16_PROPERTY_TYPE:
            case INT64_PROPERTY_TYPE:
            case DOUBLE_PROPERTY_TYPE:
            case NOMINAL_PROPERTY_TYPE:
            {
                if (!pSorter) pSorter.reset(new Sorter(pSorterCache));
                SortProperty* pSortProperty = new SortProperty(
                    iter->first,
                    propertyType,
                    iter->second);
                pSorter->addSortProperty(pSortProperty);
                break;
            }
            default:
                DLOG(ERROR) << "Sort by properties other than int, float, double type"; // TODO : Log
                break;
            }
        }
    }
}

bool SearchManagerPreProcessor::getPropertyTypeByName_(
    const std::string& name,
    PropertyDataType& type) const
{
    boost::unordered_map<std::string, PropertyConfig>::const_iterator it
    = schemaMap_.find(name);

    if (it != schemaMap_.end())
    {
        type = it->second.getType();
        return true;
    }

    return false;
}

CustomRankerPtr
SearchManagerPreProcessor::buildCustomRanker_(KeywordSearchActionItem& actionItem)
{
    CustomRankerPtr customRanker(new CustomRanker());

    customRanker->getConstParamMap() = actionItem.paramConstValueMap_;
    customRanker->getPropertyParamMap() = actionItem.paramPropertyValueMap_;

    customRanker->parse(actionItem.strExp_);

    std::map<std::string, PropertyDataType>& propertyDataTypeMap
    = customRanker->getPropertyDataTypeMap();
    std::map<std::string, PropertyDataType>::iterator iter
    = propertyDataTypeMap.begin();
    for (; iter != propertyDataTypeMap.end(); iter++)
    {
        getPropertyTypeByName_(iter->first, iter->second);
    }

    return customRanker;
}

void SearchManagerPreProcessor::fillSearchInfoWithSortPropertyData_(
    Sorter* pSorter,
    std::vector<unsigned int>& docIdList,
    DistKeywordSearchInfo& distSearchInfo,
    SortPropertyCache* pSorterCache)
{
    if (!pSorter) return;

    size_t docNum = docIdList.size();
    std::list<SortProperty*>& sortProperties = pSorter->sortProperties_;
    std::list<SortProperty*>::iterator iter;
    SortProperty* pSortProperty;

    for (iter = sortProperties.begin(); iter != sortProperties.end(); ++iter)
    {
        pSortProperty = *iter;
        std::string SortPropertyName = pSortProperty->getProperty();
        distSearchInfo.sortPropertyList_.push_back(
            std::make_pair(SortPropertyName, pSortProperty->isReverse()));

        if (SortPropertyName == "CUSTOM_RANK" || SortPropertyName == "RANK")
            continue;

        boost::shared_ptr<NumericPropertyTableBase>& numericPropertyTable = createPropertyTable(SortPropertyName, pSorterCache);
        if (!numericPropertyTable)
            continue;

        switch (numericPropertyTable->getType())
        {
        case INT32_PROPERTY_TYPE:
        case INT8_PROPERTY_TYPE:
        case INT16_PROPERTY_TYPE:
        {
            distSearchInfo.sortPropertyInt32DataList_.push_back(std::make_pair(SortPropertyName, std::vector<int32_t>()));
            std::vector<int32_t>& dataList = distSearchInfo.sortPropertyInt32DataList_.back().second;
            dataList.resize(docNum);
            for (size_t i = 0; i < docNum; i++)
            {
                numericPropertyTable->getInt32Value(docIdList[i], dataList[i]);
            }
        }
        break;
        case INT64_PROPERTY_TYPE:
        case DATETIME_PROPERTY_TYPE:
        {
            distSearchInfo.sortPropertyInt64DataList_.push_back(std::make_pair(SortPropertyName, std::vector<int64_t>()));
            std::vector<int64_t>& dataList = distSearchInfo.sortPropertyInt64DataList_.back().second;
            dataList.resize(docNum);
            for (size_t i = 0; i < docNum; i++)
            {
                numericPropertyTable->getInt64Value(docIdList[i], dataList[i]);
            }
        }
        break;
        case FLOAT_PROPERTY_TYPE:
        {
            distSearchInfo.sortPropertyFloatDataList_.push_back(std::make_pair(SortPropertyName, std::vector<float>()));
            std::vector<float>& dataList = distSearchInfo.sortPropertyFloatDataList_.back().second;
            dataList.resize(docNum);
            for (size_t i = 0; i < docNum; i++)
            {
                numericPropertyTable->getFloatValue(docIdList[i], dataList[i]);
            }
        }
        break;
        case DOUBLE_PROPERTY_TYPE:
        {
            distSearchInfo.sortPropertyFloatDataList_.push_back(std::make_pair(SortPropertyName, std::vector<float>()));
            std::vector<float>& dataList = distSearchInfo.sortPropertyFloatDataList_.back().second;
            dataList.resize(docNum);
            for (size_t i = 0; i < docNum; i++)
            {
                numericPropertyTable->getFloatValue(docIdList[i], dataList[i]);
            }
        }
        break;
        default:
            break;
        }
    }
}

void SearchManagerPreProcessor::PreparePropertyTermIndex(
    const std::map<std::string, PropertyTermInfo>& propertyTermInfoMap,
    const std::vector<std::string>& indexPropertyList,
    std::vector<std::map<termid_t, unsigned> >& termIndexMaps)
{
    // use empty object for not found property
    const PropertyTermInfo emptyPropertyTermInfo;
    // build term index maps
    typedef std::vector<std::string>::const_iterator property_list_iterator;
    for (uint32_t i = 0; i < indexPropertyList.size(); ++i)
    {
        const PropertyTermInfo::id_uint_list_map_t& termPositionsMap =
            izenelib::util::getOr(
                propertyTermInfoMap,
                indexPropertyList[i],
                emptyPropertyTermInfo
            ).getTermIdPositionMap();

        unsigned index = 0;
        typedef PropertyTermInfo::id_uint_list_map_t::const_iterator
        term_id_position_iterator;
        for (term_id_position_iterator termIt = termPositionsMap.begin();
                termIt != termPositionsMap.end(); ++termIt)
        {
            termIndexMaps[i][termIt->first] = index++;
        }
    }
}

ProductScorer* SearchManagerPreProcessor::createProductScorer(
    const KeywordSearchActionItem& actionItem,
    faceted::PropSharedLockSet& propSharedLockSet,
    ProductScorer* relevanceScorer)
{
    std::auto_ptr<ProductScorer> relevanceScorerPtr(relevanceScorer);

    if (!hasSortByRankProp(actionItem.sortPriorityList_))
        return NULL;

    if (!isProductRanking(actionItem))
        return relevanceScorerPtr.release();

    const std::string& query = actionItem.env_.queryString_;
    return productScorerFactory_->createScorer(query,
                                               propSharedLockSet,
                                               relevanceScorerPtr.release());
}

bool SearchManagerPreProcessor::isProductRanking(
    const KeywordSearchActionItem& actionItem) const
{
    if (productScorerFactory_ == NULL)
        return false;

    SearchingMode::SearchingModeType searchMode =
        actionItem.searchingMode_.mode_;

    if (searchMode == SearchingMode::KNN)
        return false;

    return true;
}

bool SearchManagerPreProcessor::isNeedCustomDocIterator(
    const KeywordSearchActionItem& actionItem) const
{
    return hasSortByRankProp(actionItem.sortPriorityList_) &&
        isProductRanking(actionItem);
}

bool SearchManagerPreProcessor::isNeedRerank(
    const KeywordSearchActionItem& actionItem) const
{
    return isSortByRankProp(actionItem.sortPriorityList_) &&
        isProductRanking(actionItem);
}

bool SearchManagerPreProcessor::hasSortByRankProp(
    const KeywordSearchActionItem::SortPriorityList& sortPriorityList)
{
    for (KeywordSearchActionItem::SortPriorityList::const_iterator it =
             sortPriorityList.begin(); it != sortPriorityList.end(); ++it)
    {
        std::string propName = it->first;
        boost::to_lower(propName);
        if (propName == RANK_PROPERTY)
            return true;
    }

    return false;
}

bool SearchManagerPreProcessor::isSortByRankProp(
    const KeywordSearchActionItem::SortPriorityList& sortPriorityList)
{
    return sortPriorityList.size() == 1 &&
        hasSortByRankProp(sortPriorityList);
}

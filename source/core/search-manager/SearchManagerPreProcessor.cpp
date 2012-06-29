#include "SearchManagerPreProcessor.h"
#include "Sorter.h"
#include "NumericPropertyTable.h"
#include "NumericPropertyTableBuilder.h"

#include <mining-manager/MiningManager.h>
#include <common/SFLogger.h>
#include <util/get.h>
#include <util/ClockTimer.h>
#include <fstream>


namespace sf1r {

const std::string RANK_PROPERTY("_rank");
const std::string DATE_PROPERTY("date");
const std::string CUSTOM_RANK_PROPERTY("custom_rank");
const std::string CTR_PROPERTY("_ctr");

SearchManagerPreProcessor::SearchManagerPreProcessor()
    : schemaMap_()
{
}

SearchManagerPreProcessor::~SearchManagerPreProcessor()
{
}

NumericPropertyTable*
SearchManagerPreProcessor::createPropertyTable(const std::string& propertyName, SortPropertyCache* psortercache)
{
    boost::shared_ptr<PropertyData> propData = getPropertyData_(propertyName, psortercache);
    if (propData)
    {
        return new NumericPropertyTable(propertyName, propData);
    }

    return NULL;
}

void SearchManagerPreProcessor::prepare_sorter_customranker_(
    const SearchKeywordOperation& actionOperation,
    CustomRankerPtr& customRanker,
    boost::shared_ptr<Sorter> &pSorter,
    SortPropertyCache* psortercache,
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
                if (!customRanker->setPropertyData(psortercache))
                {
                    LOG(ERROR) << customRanker->getErrorInfo() ;
                    continue;
                }
                //customRanker->printESTree();

                if (!pSorter) pSorter.reset(new Sorter(psortercache));
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
                if (!pSorter) pSorter.reset(new Sorter(psortercache));
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
                if (!pSorter) pSorter.reset(new Sorter(psortercache));
                SortProperty* pSortProperty = new SortProperty(
                        iter->first,
                        INT_PROPERTY_TYPE,
                        iter->second);
                pSorter->addSortProperty(pSortProperty);
                continue;
            }
            // sort by ctr (click through rate)
            if (fieldNameL == CTR_PROPERTY)
            {
                if (miningManagerPtr.expired())
                {
                    DLOG(ERROR)<<"Skipped CTR sort property: Mining Manager was not initialized";
                    continue;
                }
                boost::shared_ptr<MiningManager> resourceMiningManagerPtr = miningManagerPtr.lock();
                boost::shared_ptr<faceted::CTRManager> ctrManangerPtr
                = resourceMiningManagerPtr->GetCtrManager();
                if (!ctrManangerPtr)
                {
                    DLOG(ERROR)<<"Skipped CTR sort property: CTR Manager was not initialized";
                    continue;
                }

                psortercache->setCtrManager(ctrManangerPtr.get());
                if (!pSorter) pSorter.reset(new Sorter(psortercache));

                SortProperty* pSortProperty = new SortProperty(
                        iter->first,
                        UNSIGNED_INT_PROPERTY_TYPE,
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
            case INT_PROPERTY_TYPE:
            case FLOAT_PROPERTY_TYPE:
            case NOMINAL_PROPERTY_TYPE:
            case UNSIGNED_INT_PROPERTY_TYPE:
            case DOUBLE_PROPERTY_TYPE:
            case STRING_PROPERTY_TYPE:
            case DATETIME_PROPERTY_TYPE:
            {
                if (!pSorter) pSorter.reset(new Sorter(psortercache));
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
        SortPropertyCache* psortercache)
{
    if (!pSorter)
        return;

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

        boost::shared_ptr<PropertyData> propData = getPropertyData_(SortPropertyName, psortercache);
        if (!propData)
            continue;

        void* data = propData->data_;
        size_t size = propData->size_;
        switch (propData->type_)
        {
        case INT_PROPERTY_TYPE:
        case DATETIME_PROPERTY_TYPE:
            {
                std::vector<int64_t> dataList(docNum);
                for (size_t i = 0; i < docNum; i++)
                {
                    if (docIdList[i] >= size) dataList[i] = 0;
                    else dataList[i] = ((int64_t*) data)[docIdList[i]];
                }
                distSearchInfo.sortPropertyIntDataList_.push_back(
                        std::make_pair(SortPropertyName, dataList));
            }
            break;
        case UNSIGNED_INT_PROPERTY_TYPE:
            {
                std::vector<uint64_t> dataList(docNum);
                for (size_t i = 0; i < docNum; i++)
                {
                    if (docIdList[i] >= size) dataList[i] = 0;
                    else dataList[i] = ((uint64_t*) data)[docIdList[i]];
                }
                distSearchInfo.sortPropertyUIntDataList_.push_back(std::make_pair(SortPropertyName, dataList));
            }
            break;
        case FLOAT_PROPERTY_TYPE:
            {
                std::vector<float> dataList(docNum);
                for (size_t i = 0; i < docNum; i++)
                {
                    if (docIdList[i] >= size) dataList[i] = 0.0;
                    else dataList[i] = ((float*) data)[docIdList[i]];
                }
                distSearchInfo.sortPropertyFloatDataList_.push_back(std::make_pair(SortPropertyName, dataList));
            }
            break;
        case DOUBLE_PROPERTY_TYPE:
            {
                std::vector<float> dataList(docNum);
                for (size_t i = 0; i < docNum; i++)
                {
                    if (docIdList[i] >= size) dataList[i] = 0.0;
                    else dataList[i] = (float)((double*) data)[docIdList[i]];
                }
                distSearchInfo.sortPropertyFloatDataList_.push_back(std::make_pair(SortPropertyName, dataList));
            }
            break;
        default:
            break;
        }
    }
}

boost::shared_ptr<PropertyData>
SearchManagerPreProcessor::getPropertyData_(const std::string& name, SortPropertyCache* psortercache)
{
    boost::shared_ptr<PropertyData> propData;
    PropertyDataType type = UNKNOWN_DATA_PROPERTY_TYPE;

    if (getPropertyTypeByName_(name, type))
    {
        propData = psortercache->getSortPropertyData(name, type);
    }

    return propData;
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

}

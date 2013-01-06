#include "Sorter.h"

#include <document-manager/DocumentManager.h>
#include <index-manager/IndexManager.h>
#include <mining-manager/faceted-submanager/ctr_manager.h>
#include <bundles/index/IndexBundleConfiguration.h>
#include <common/PropSharedLockSet.h>

namespace sf1r
{

const char SortPropertyCache::Separator[] = {'-', '~', ','};

SortProperty::SortProperty(const SortProperty& src)
    : property_(src.property_)
    , propertyDataType_(src.propertyDataType_)
    , type_(src.type_)
    , reverse_(src.reverse_)
    , pComparator_(src.pComparator_)
{
}

SortProperty::SortProperty(const string& property, PropertyDataType propertyType, bool reverse)
    : property_(property)
    , propertyDataType_(propertyType)
    , type_(AUTO)
    , reverse_(reverse)
    , pComparator_(NULL)
{
}

SortProperty::SortProperty(const string& property, PropertyDataType propertyType, SortPropertyType type, bool reverse)
    : property_(property)
    , propertyDataType_(propertyType)
    , type_(type)
    , reverse_(reverse)
    , pComparator_(NULL)
{
}
SortProperty::SortProperty(const string& property, PropertyDataType propertyType, SortPropertyComparator* pComparator, bool reverse)
    : property_(property)
    , propertyDataType_(propertyType)
    , type_(CUSTOM)
    , reverse_(reverse)
    , pComparator_(pComparator)
{
}

SortProperty::SortProperty(const string& property, PropertyDataType propertyType, SortPropertyComparator* pComparator, SortPropertyType type, bool reverse)
    : property_(property)
    , propertyDataType_(propertyType)
    , type_(type)
    , reverse_(reverse)
    , pComparator_(pComparator)
{
}

SortProperty::~SortProperty()
{
    if (pComparator_)
        delete pComparator_;
}

SortPropertyCache::SortPropertyCache(DocumentManager* pDocumentManager, IndexManager* pIndexer, IndexBundleConfiguration* config)
    : pDocumentManager_(pDocumentManager)
    , pIndexer_(pIndexer)
    , pCTRManager_(NULL)
    , updateInterval_(config->sortCacheUpdateInterval_)
    , dirty_(true)
    , config_(config)
{
    std::cout << "SortPropertyCache::updateInterval_ = " << updateInterval_ << std::endl;
}

void SortPropertyCache::setCtrManager(faceted::CTRManager* pCTRManager)
{
    pCTRManager_ = pCTRManager;
}

void SortPropertyCache::loadSortData(const std::string& property, PropertyDataType type)
{
    boost::shared_ptr<NumericPropertyTableBase>& numericPropertyTable = sortDataCache_[property];
    if (type == STRING_PROPERTY_TYPE)
    {
        std::size_t size = pIndexer_->getIndexReader()->maxDoc();
        if (size)
        {
            pIndexer_->convertStringPropertyDataForSorting(property, numericPropertyTable);
        }
    }
    else
    {
        boost::shared_ptr<NumericPropertyTableBase>& tempTable = pDocumentManager_->getNumericPropertyTable(property);
        if (tempTable)
        {
            numericPropertyTable = tempTable;
        }
        else
        {
            switch (type)
            {
            case INT32_PROPERTY_TYPE:
            case INT8_PROPERTY_TYPE:
            case INT16_PROPERTY_TYPE:
                pIndexer_->loadPropertyDataForSorting<int32_t>(property, type, numericPropertyTable);
                break;

            case FLOAT_PROPERTY_TYPE:
                pIndexer_->loadPropertyDataForSorting<float>(property, type, numericPropertyTable);
                break;

            case INT64_PROPERTY_TYPE:
            case DATETIME_PROPERTY_TYPE:
                pIndexer_->loadPropertyDataForSorting<int64_t>(property, type, numericPropertyTable);
                break;

            case DOUBLE_PROPERTY_TYPE:
                pIndexer_->loadPropertyDataForSorting<double>(property, type, numericPropertyTable);
                break;

            default:
                break;
            }
        }
    }
}

boost::shared_ptr<NumericPropertyTableBase>& SortPropertyCache::getSortPropertyData(const std::string& propertyName, PropertyDataType propertyType)
{
    boost::mutex::scoped_lock lock(this->mutex_);
    if (dirty_)
    {
        for (SortDataCache::iterator iter = sortDataCache_.begin();
                iter != sortDataCache_.end(); ++iter)
        {
            LOG(INFO) << "dirty sort data cache on property: " << iter->first;
            if (iter->first == faceted::CTRManager::kCtrPropName) continue;
            if (iter->second) loadSortData(iter->first, iter->second->getType());
        }
        dirty_ = false;
    }

    SortDataCache::iterator iter = sortDataCache_.find(propertyName);
    if (iter == sortDataCache_.end() || !iter->second)
    {
        LOG(INFO) << "first load sort data cache on property: " << propertyName;
        loadSortData(propertyName, propertyType);
    }

    return sortDataCache_[propertyName];
}

boost::shared_ptr<NumericPropertyTableBase>& SortPropertyCache::getCTRPropertyData(const std::string& propertyName, PropertyDataType propertyType)
{
    boost::mutex::scoped_lock lock(this->mutex_);

    boost::shared_ptr<NumericPropertyTableBase>& numericPropertyTable = sortDataCache_[propertyName];
    if (!numericPropertyTable)
        pCTRManager_->loadCtrData(numericPropertyTable);

    return numericPropertyTable;
}

SortPropertyComparator* SortPropertyCache::getComparator(
    SortProperty* pSortProperty,
    PropSharedLockSet& propSharedLockSet)
{
    SortProperty::SortPropertyType propSortType = pSortProperty->getType();
    const std::string& propName = pSortProperty->getProperty();
    const PropertyDataType propDataType = pSortProperty->getPropertyDataType();

    boost::shared_ptr<NumericPropertyTableBase> numericPropertyTable;
    if (propSortType == SortProperty::AUTO)
    {
        numericPropertyTable = getSortPropertyData(propName, propDataType);
    }
    else if (propSortType == SortProperty::CTR)
    {
        if (pCTRManager_)
            numericPropertyTable = getCTRPropertyData(propName, propDataType);
    }
    else
    {
        // to extend
    }

    if (numericPropertyTable)
    {
        propSharedLockSet.insertSharedLock(numericPropertyTable.get());
        return new SortPropertyComparator(numericPropertyTable);
    }

    return NULL;
}

Sorter::Sorter(SortPropertyCache* pCache)
    : pCache_(pCache)
    , ppSortProperties_(0)
    , reverseMul_(0)
{
}

Sorter::~Sorter()
{
    for (std::list<SortProperty*>::iterator iter = sortProperties_.begin();
            iter != sortProperties_.end(); ++iter)
        delete *iter;
    if (ppSortProperties_) delete[] ppSortProperties_;
    if (reverseMul_) delete[] reverseMul_;
}

void Sorter::addSortProperty(const string& property, PropertyDataType propertyType, bool reverse)
{
    SortProperty* pSortProperty = new SortProperty(property, propertyType, reverse);
    sortProperties_.push_back(pSortProperty);
}

void Sorter::addSortProperty(SortProperty* pSortProperty)
{
    sortProperties_.push_back(pSortProperty);
}

void Sorter::getComparators(PropSharedLockSet& propSharedLockSet)
{
    SortProperty* pSortProperty;
    nNumProperties_ = sortProperties_.size();
    ppSortProperties_ = new SortProperty*[nNumProperties_];
    size_t i = 0;
    size_t numOfInValidComparators = 0;
    for (std::list<SortProperty*>::iterator iter = sortProperties_.begin();
            iter != sortProperties_.end(); ++iter, ++i)
    {
        pSortProperty = *iter;
        ppSortProperties_[i] = pSortProperty;
        if (pSortProperty)
        {
            switch (pSortProperty->getType())
            {
            case SortProperty::SCORE:
                pSortProperty->pComparator_= new SortPropertyComparator();
                break;
            case SortProperty::AUTO:
                if (!pSortProperty->pComparator_)
                    pSortProperty->pComparator_ = pCache_->getComparator(pSortProperty,
                                                                         propSharedLockSet);
                if (!pSortProperty->pComparator_)
                    ++numOfInValidComparators;
                break;
            case SortProperty::CUSTOM:
                pSortProperty->pComparator_ = new SortPropertyComparator(CUSTOM_RANKING_PROPERTY_TYPE);
                break;
            case SortProperty::CTR:
                pSortProperty->pComparator_ = pCache_->getComparator(pSortProperty,
                                                                     propSharedLockSet);
                if (!pSortProperty->pComparator_)
                    ++numOfInValidComparators;
                break;
            }
        }
    }
    if (numOfInValidComparators > 0)
    {
        SortProperty** ppSortProperties = NULL;
        if (numOfInValidComparators == nNumProperties_)
        {
            nNumProperties_ = 1;
            ppSortProperties = new SortProperty*[nNumProperties_];
            SortProperty* pSortProperty = new SortProperty("RANK", UNKNOWN_DATA_PROPERTY_TYPE);
            pSortProperty->pComparator_ = new SortPropertyComparator();
            ppSortProperties[0] = pSortProperty;
            sortProperties_.push_back(pSortProperty);
        }
        else
        {
            ppSortProperties = new SortProperty*[nNumProperties_];
            size_t j = 0;
            for (i = 0; i < nNumProperties_; ++i)
            {
                SortProperty* pSortProperty = ppSortProperties_[i];
                if (pSortProperty->pComparator_)
                {
                    ppSortProperties[j++] = pSortProperty;
                }
            }
            nNumProperties_ = j;
        }
        delete [] ppSortProperties_;
        ppSortProperties_ = ppSortProperties;
    }
    reverseMul_ = new int[nNumProperties_];
    for (i = 0; i < nNumProperties_; ++i)
    {
        reverseMul_[i] = ppSortProperties_[i]->isReverse() ? -1 : 1;
    }
}

}

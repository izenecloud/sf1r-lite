#include "Sorter.h"

using namespace sf1r;

SortProperty::SortProperty(const SortProperty& src)
        :property_(src.property_)
        ,propertyDataType_(src.propertyDataType_)
        ,type_(src.type_)
        ,reverse_(src.reverse_)
        ,pComparator_(src.pComparator_)
{
}

SortProperty::SortProperty(const string& property, PropertyDataType propertyType, bool reverse)
        :property_(property)
        ,propertyDataType_(propertyType)
        ,type_(AUTO)
        ,reverse_(reverse)
        ,pComparator_(NULL)
{
}

SortProperty::SortProperty(const string& property, PropertyDataType propertyType, SortPropertyType type, bool reverse)
        :property_(property)
        ,propertyDataType_(propertyType)
        ,type_(type)
        ,reverse_(reverse)
        ,pComparator_(NULL)
{
}
SortProperty::SortProperty(const string& property, PropertyDataType propertyType, SortPropertyComparator* pComparator, bool reverse)
        :property_(property)
        ,propertyDataType_(propertyType)
        ,type_(CUSTOM)
        ,reverse_(reverse)
        ,pComparator_(pComparator)
{
}

SortProperty::~SortProperty()
{
    if (pComparator_)
        delete pComparator_;
}

SortPropertyCache::SortPropertyCache(IndexManager* pIndexer)
        :pIndexer_(pIndexer)
        ,dirty_(true)
{
}

SortPropertyCache::~SortPropertyCache()
{
    clear();
}

void SortPropertyCache::clear()
{
    for (std::map<std::string, std::pair<PropertyDataType,void*> >::iterator iter = sortDataCache_.begin();
            iter != sortDataCache_.end(); ++iter)
    {
        void* data = iter->second.second;
        if (data == NULL)
            continue;
        switch (iter->second.first)
        {
        case INT_PROPERTY_TYPE:
            delete[] (int64_t*)data;
            break;
        case UNSIGNED_INT_PROPERTY_TYPE:
            delete[] (uint64_t*)data;
            break;
        case FLOAT_PROPERTY_TYPE:
            delete[] (float*)data;
            break;
        case DOUBLE_PROPERTY_TYPE:
            delete[] (double*)data;
            break;
        default:
            break;
        }
    }
}

void SortPropertyCache::clear(const std::string& property)
{
    std::map<std::string, std::pair<PropertyDataType,void*> >::iterator iter = sortDataCache_.find(property);

    if(iter != sortDataCache_.end())
    {
        void* data = iter->second.second;
        if (data == NULL)
            return;
        switch (iter->second.first)
        {
        case INT_PROPERTY_TYPE:
            delete[] (int64_t*)data;
            break;
        case UNSIGNED_INT_PROPERTY_TYPE:
            delete[] (uint64_t*)data;
            break;
        case FLOAT_PROPERTY_TYPE:
            delete[] (float*)data;
            break;
        case DOUBLE_PROPERTY_TYPE:
            delete[] (double*)data;
            break;
        default:
            break;
        }
    }
}

void SortPropertyCache::loadSortData(const std::string& property, PropertyDataType type)
{
    clear(property);
    switch (type)
    {
    case INT_PROPERTY_TYPE:
    {
        int64_t* data = NULL;
        pIndexer_->loadPropertyDataForSorting(property, data);
        sortDataCache_[property] = make_pair(INT_PROPERTY_TYPE, data);
    }
        break;
    case UNSIGNED_INT_PROPERTY_TYPE:
    {
        uint64_t* data = NULL;
        pIndexer_->loadPropertyDataForSorting(property, data);
        sortDataCache_[property] = make_pair(UNSIGNED_INT_PROPERTY_TYPE, data);
    }
        break;
    case FLOAT_PROPERTY_TYPE:
    {
        float* data = NULL;
        pIndexer_->loadPropertyDataForSorting(property, data);
        sortDataCache_[property] = make_pair(FLOAT_PROPERTY_TYPE, data);
    }
        break;
    case DOUBLE_PROPERTY_TYPE:
    {
        double* data = NULL;
        pIndexer_->loadPropertyDataForSorting(property, data);
        sortDataCache_[property] = make_pair(DOUBLE_PROPERTY_TYPE, data);
    }
        break;
    default:
        break;
    }
}

SortPropertyComparator* SortPropertyCache::getComparator(SortProperty* pSortProperty)
{
    boost::mutex::scoped_lock lock(this->mutex_);

    if (dirty_)
    {
        for (std::map<std::string, std::pair<PropertyDataType,void*> >::iterator iter = sortDataCache_.begin();
                iter != sortDataCache_.end(); ++iter)
        {
            cout<<"dirty "<<iter->first<<endl;
            loadSortData(iter->first, iter->second.first);
        }
        dirty_ = false;
    }

    std::map<std::string, std::pair<PropertyDataType,void*> >::iterator iter = sortDataCache_.find(pSortProperty->getProperty()) ;
    if (iter == sortDataCache_.end())
    {
        void* data = NULL;
        sortDataCache_[pSortProperty->getProperty()] = make_pair(pSortProperty->getPropertyDataType(), data);
        cout<<"first load "<<pSortProperty->getProperty()<<endl;
        loadSortData(pSortProperty->getProperty(), pSortProperty->getPropertyDataType());
    }
    return new SortPropertyComparator(sortDataCache_[pSortProperty->getProperty()].second, pSortProperty->getPropertyDataType());
}

Sorter::Sorter(SortPropertyCache* pCache)
        :pCache_(pCache)
        ,ppSortProperties_(0)
{
}

Sorter::~Sorter()
{
    for (std::list<SortProperty*>::iterator iter = sortProperties_.begin();
            iter != sortProperties_.end(); ++iter)
        delete *iter;
    if (ppSortProperties_)
    {
        delete[] ppSortProperties_;
        ppSortProperties_ = NULL;
    }
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

void Sorter::getComparators()
{
    SortProperty* pSortProperty;
    nNumProperties_ = sortProperties_.size();
    ppSortProperties_ = new SortProperty*[nNumProperties_];
    size_t i = 0;
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
                pSortProperty->pComparator_ = pCache_->getComparator(pSortProperty);
                break;
            case SortProperty::CUSTOM:
                assert(pSortProperty->pComparator_);
                break;
            }
        }
    }
}


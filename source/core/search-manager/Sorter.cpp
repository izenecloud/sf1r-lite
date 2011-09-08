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

SortPropertyCache::SortPropertyCache(IndexManager* pIndexer, IndexBundleConfiguration* config)
        :pIndexer_(pIndexer)
        ,dirty_(true)
        ,config_(config)
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

bool SortPropertyCache::getSortPropertyData(const std::string& propertyName, PropertyDataType propertyType, void* &data)
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

    std::map<std::string, std::pair<PropertyDataType,void*> >::iterator iter = sortDataCache_.find(propertyName) ;
    if (iter == sortDataCache_.end())
    {
        void* data = NULL;
        sortDataCache_[propertyName] = make_pair(propertyType, data);
        cout << "first load <" << propertyName << ">" << endl;
        loadSortData(propertyName, propertyType);
    }

    data = sortDataCache_[propertyName].second;

    if (data != NULL)
        return true;
    else
        return false;
}

void SortPropertyCache::updateSortData(docid_t id, const std::map<std::string, pair<PropertyDataType, izenelib::util::UString> >& rTypeFieldValue)
{
    boost::mutex::scoped_lock lock(this->mutex_);

    izenelib::util::UString::EncodingType encoding = config_->encoding_;
    std::map<std::string, pair<PropertyDataType, izenelib::util::UString> >::const_iterator iter;
    std::map<std::string, std::pair<PropertyDataType,void*> >::const_iterator it;
    for(iter = rTypeFieldValue.begin(); iter != rTypeFieldValue.end(); iter++)
    {
        string propertyName = iter->first;
        PropertyDataType dataType = iter->second.first;
        switch(dataType)
        {
        case INT_PROPERTY_TYPE:
            {
                std::string str("");
                iter->second.second.convertString(str, encoding);
                int64_t value = 0;
                int64_t* data;
                value = boost::lexical_cast< int64_t >( str );
                if ((it = sortDataCache_.find(propertyName)) != sortDataCache_.end())
                {
                    data = (int64_t*)(it->second.second);
                    data[id] = value;
                }
            }
            break;
        case UNSIGNED_INT_PROPERTY_TYPE:
            {
                std::string str("");
                iter->second.second.convertString(str, encoding);
                uint64_t value = 0;
                uint64_t* data;
                value = boost::lexical_cast< uint64_t >( str );
                if ((it = sortDataCache_.find(propertyName)) != sortDataCache_.end())
                {
                    data = (uint64_t*)(it->second.second);
                    data[id] = value;
                }
            }
            break;
        case FLOAT_PROPERTY_TYPE:
            {
                std::string str("");
                iter->second.second.convertString(str, encoding);
                float value = 0.0;
                float* data;
                value = boost::lexical_cast< float >( str );
                if ((it = sortDataCache_.find(propertyName)) != sortDataCache_.end())
                {
                    data = (float*)(it->second.second);
                    data[id] = value;
                }
            }
            break;
        case DOUBLE_PROPERTY_TYPE:
            {
                std::string str("");
                iter->second.second.convertString(str, encoding);
                double value = 0.0;
                double* data;
                value = boost::lexical_cast< double >( str );
                if ((it = sortDataCache_.find(propertyName)) != sortDataCache_.end())
                {
                    data = (double*)(it->second.second);
                    data[id] = value;
                }
            }
            break;
        default:
              break;
        }
    }
}

SortPropertyComparator* SortPropertyCache::getComparator(SortProperty* pSortProperty)
{
    void *data;
    getSortPropertyData(pSortProperty->getProperty(), pSortProperty->getPropertyDataType(), data);

    return new SortPropertyComparator(data, pSortProperty->getPropertyDataType());
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
                pSortProperty->pComparator_ = new SortPropertyComparator(CUSTOM_RANKING_PROPERTY_TYPE);
                break;
            }
        }
    }
}


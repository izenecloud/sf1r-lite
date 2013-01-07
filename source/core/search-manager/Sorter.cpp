#include "Sorter.h"
#include "NumericPropertyTableBuilder.h"

#include <document-manager/DocumentManager.h>
#include <index-manager/IndexManager.h>
#include <bundles/index/IndexBundleConfiguration.h>
#include <common/PropSharedLockSet.h>

namespace sf1r
{

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

Sorter::Sorter(NumericPropertyTableBuilder* numericTableBuilder)
    : numericTableBuilder_(numericTableBuilder)
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

void Sorter::createComparators(PropSharedLockSet& propSharedLockSet)
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
        const std::string& propName = pSortProperty->getProperty();

        if (pSortProperty)
        {
            switch (pSortProperty->getType())
            {
            case SortProperty::SCORE:
                pSortProperty->pComparator_= new SortPropertyComparator();
                break;
            case SortProperty::AUTO:
                if (!pSortProperty->pComparator_)
                    pSortProperty->pComparator_ = createNumericComparator_(propName,
                                                                           propSharedLockSet);
                if (!pSortProperty->pComparator_)
                    ++numOfInValidComparators;
                break;
            case SortProperty::CUSTOM:
                pSortProperty->pComparator_ = new SortPropertyComparator(CUSTOM_RANKING_PROPERTY_TYPE);
                break;
            case SortProperty::CTR:
                pSortProperty->pComparator_ = createNumericComparator_(propName,
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

SortPropertyComparator* Sorter::createNumericComparator_(
    const std::string& propName,
    PropSharedLockSet& propSharedLockSet)
{
    if (!numericTableBuilder_)
        return NULL;

    boost::shared_ptr<NumericPropertyTableBase> numericTable =
        numericTableBuilder_->createPropertyTable(propName);

    if (!numericTable)
        return NULL;

    propSharedLockSet.insertSharedLock(numericTable.get());
    return new SortPropertyComparator(numericTable);
}

}

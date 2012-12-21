#include "SortPropertyComparator.h"

namespace sf1r
{

SortPropertyComparator::SortPropertyComparator()
    : propertyTableLock_(NULL)
    , type_(UNKNOWN_DATA_PROPERTY_TYPE)
    , size_(0)
{
    initComparator();
}

SortPropertyComparator::SortPropertyComparator(const boost::shared_ptr<NumericPropertyTableBase>& propData)
    : propertyTable_(propData)
    , propertyTableLock_(& propertyTable_->mutex_)
    , type_(propData->getType())
    , size_(propData->size())
{
    propertyTableLock_->lock_shared();
    initComparator();
}

SortPropertyComparator::SortPropertyComparator(PropertyDataType dataType)
    : propertyTableLock_(NULL)
    , type_(dataType)
    , size_(0)
{
    initComparator();
}

SortPropertyComparator::~SortPropertyComparator()
{
    if(propertyTableLock_)
    {
        propertyTableLock_->unlock_shared();
    }
}

void SortPropertyComparator::initComparator()
{
    switch (type_)
    {
    case STRING_PROPERTY_TYPE:
    case INT32_PROPERTY_TYPE:
    case FLOAT_PROPERTY_TYPE:
    case DATETIME_PROPERTY_TYPE:
    case INT8_PROPERTY_TYPE:
    case INT16_PROPERTY_TYPE:
    case INT64_PROPERTY_TYPE:
    case DOUBLE_PROPERTY_TYPE:
        comparator_ = &SortPropertyComparator::compareImplNumeric;
        break;
    case UNKNOWN_DATA_PROPERTY_TYPE:
        comparator_ = &SortPropertyComparator::compareImplUnknown;
        break;
    case CUSTOM_RANKING_PROPERTY_TYPE:
        comparator_ = &SortPropertyComparator::compareImplCustomRanking;
        break;
    default:
        comparator_ = &SortPropertyComparator::compareImplDefault;
        break;
    }
}

int SortPropertyComparator::compare(const ScoreDoc& doc1, const ScoreDoc& doc2) const
{
    return (this->*comparator_)(doc1, doc2);
}

int SortPropertyComparator::compareImplDefault(const ScoreDoc& doc1, const ScoreDoc& doc2) const
{
    return 0;
}

int SortPropertyComparator::compareImplNumeric(const ScoreDoc& doc1, const ScoreDoc& doc2) const
{
    if (doc1.docId >= size_ || doc2.docId >= size_) return 0;
    return propertyTable_->compareValues(doc1.docId, doc2.docId);
}

int SortPropertyComparator::compareImplUnknown(const ScoreDoc& doc1, const ScoreDoc& doc2) const
{
    if (doc1.score < doc2.score) return -1;
    if (doc1.score > doc2.score) return 1;
    return 0;
}

int SortPropertyComparator::compareImplCustomRanking(const ScoreDoc& doc1, const ScoreDoc& doc2) const
{
    if (doc1.custom_score < doc2.custom_score) return -1;
    if (doc1.custom_score > doc2.custom_score) return 1;
    return 0;
}

}

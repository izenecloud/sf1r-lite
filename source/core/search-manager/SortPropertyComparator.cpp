#include "SortPropertyComparator.h"
#include <common/RTypeStringPropTable.h>

namespace sf1r
{

SortPropertyComparator::SortPropertyComparator()
    : type_(UNKNOWN_DATA_PROPERTY_TYPE)
    , size_(0)
{
    initComparator();
}

SortPropertyComparator::SortPropertyComparator(const boost::shared_ptr<NumericPropertyTableBase>& propData)
    : numericPropTable_(propData)
    , type_(propData->getType())
    , size_(propData->size(false))
{
    initComparator();
}

SortPropertyComparator::SortPropertyComparator(const boost::shared_ptr<RTypeStringPropTable>& propData)
    : RTypePropTable_(propData)
    , type_(propData->getType())
    , size_(propData->size(false))
{
    initComparator();
}

SortPropertyComparator::SortPropertyComparator(PropertyDataType dataType)
    : type_(dataType)
    , size_(0)
{
    initComparator();
}

void SortPropertyComparator::initComparator()
{
    switch (type_)
    {
    case STRING_PROPERTY_TYPE:
        comparator_ = &SortPropertyComparator::compareImplRTypeString;
        break;
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
int SortPropertyComparator::compareImplRTypeString(const ScoreDoc& doc1, const ScoreDoc& doc2) const
{
    if (doc1.docId >= size_ || doc2.docId >= size_) return 0;
    return RTypePropTable_->compareValues(doc1.docId, doc2.docId, false);
}
int SortPropertyComparator::compareImplNumeric(const ScoreDoc& doc1, const ScoreDoc& doc2) const
{
    if (doc1.docId >= size_ || doc2.docId >= size_) return 0;
    return numericPropTable_->compareValues(doc1.docId, doc2.docId, false);
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

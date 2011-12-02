#include "SortPropertyComparator.h"

namespace sf1r{

SortPropertyComparator::SortPropertyComparator()
    : type_(UNKNOWN_DATA_PROPERTY_TYPE)
    , data_(NULL)
    , size_(0)
{
    initComparator();
}

SortPropertyComparator::SortPropertyComparator(boost::shared_ptr<PropertyData> propData)
    : propertyData_(propData)
    , type_(propData->type_)
    , data_(propData->data_)
    , size_(propData->size_)
{
    initComparator();
}

SortPropertyComparator::SortPropertyComparator(PropertyDataType dataType)
    : type_(dataType)
    , data_(NULL)
    , size_(0)
{
    initComparator();
}

void SortPropertyComparator::initComparator()
{
    switch(type_)
    {
    case INT_PROPERTY_TYPE:
        comparator_ = &SortPropertyComparator::compareImplInt;
        break;
    case UNSIGNED_INT_PROPERTY_TYPE:
        comparator_ = &SortPropertyComparator::compareImplUnsigned;
        break;
    case FLOAT_PROPERTY_TYPE:
        comparator_ = &SortPropertyComparator::compareImplFloat;
        break;
    case DOUBLE_PROPERTY_TYPE:
        comparator_ = &SortPropertyComparator::compareImplDouble;
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

int SortPropertyComparator::compare(ScoreDoc doc1, ScoreDoc doc2)
{
    return (this->*comparator_)(doc1, doc2);
}

int SortPropertyComparator::compareImplDefault(ScoreDoc doc1, ScoreDoc doc2)
{
    return 0;
}

int SortPropertyComparator::compareImplInt(ScoreDoc doc1, ScoreDoc doc2)
{
    if((doc1.docId >= size_)||(doc2.docId >= size_)) return 0;
    int64_t f1 = ((int64_t*)data_)[doc1.docId];
    int64_t f2 = ((int64_t*)data_)[doc2.docId];
    if (f1 < f2) return -1;
    if (f1 > f2) return 1;
    return 0;
}

int SortPropertyComparator::compareImplUnsigned(ScoreDoc doc1, ScoreDoc doc2)
{
    if((doc1.docId >= size_)||(doc2.docId >= size_)) return 0;
    uint64_t f1 = ((uint64_t*)data_)[doc1.docId];
    uint64_t f2 = ((uint64_t*)data_)[doc2.docId];
    if (f1 < f2) return -1;
    if (f1 > f2) return 1;
    return 0;
}

int SortPropertyComparator::compareImplFloat(ScoreDoc doc1, ScoreDoc doc2)
{
    if((doc1.docId >= size_)||(doc2.docId >= size_)) return 0;
    float f1 = ((float*)data_)[doc1.docId];
    float f2 = ((float*)data_)[doc2.docId];
    if (f1 < f2) return -1;
    if (f1 > f2) return 1;
    return 0;
}

int SortPropertyComparator::compareImplDouble(ScoreDoc doc1, ScoreDoc doc2)
{
    if((doc1.docId >= size_)||(doc2.docId >= size_)) return 0;
    double f1 = ((double*)data_)[doc1.docId];
    double f2 = ((double*)data_)[doc2.docId];
    if (f1 < f2) return -1;
    if (f1 > f2) return 1;
    return 0;
}

int SortPropertyComparator::compareImplUnknown(ScoreDoc doc1, ScoreDoc doc2)
{
    if (doc1.score < doc2.score) return -1;
    if (doc1.score > doc2.score) return 1;
    return 0;
}

int SortPropertyComparator::compareImplCustomRanking(ScoreDoc doc1, ScoreDoc doc2)
{
    if (doc1.custom_score < doc2.custom_score) return -1;
    if (doc1.custom_score > doc2.custom_score) return 1;
    return 0;
}

}

/**
 * @file sf1r/search-manager/SortPropertyComparator.h
 * @author Yingfeng Zhang
 * @author August Njam Grong
 * @date Created <2009-10-10>
 * @date Updated <2011-08-29>
 * @brief WildcardDocumentIterator DocumentIterator for wildcard query
 */
#ifndef SORT_PROPERTY_COMPARATOR_H
#define SORT_PROPERTY_COMPARATOR_H

#include <common/type_defs.h>
#include "ScoreDoc.h"
#include "CustomRanker.h"

namespace sf1r{

class CustomRanker;

class SortPropertyComparator
{
public:
    int compare(ScoreDoc doc1, ScoreDoc doc2);

private:
    void* data_;
    PropertyDataType type_;

public:
    SortPropertyComparator();
    SortPropertyComparator(void* data, PropertyDataType type);
    SortPropertyComparator(PropertyDataType dataType);
    ~SortPropertyComparator() {}

private:
    void initComparator();
    int (SortPropertyComparator::*comparator_)(ScoreDoc doc1, ScoreDoc doc2);
    int compareImplDefault(ScoreDoc doc1, ScoreDoc doc2);
    int compareImplInt(ScoreDoc doc1, ScoreDoc doc2);
    int compareImplUnsigned(ScoreDoc doc1, ScoreDoc doc2);
    int compareImplFloat(ScoreDoc doc1, ScoreDoc doc2);
    int compareImplDouble(ScoreDoc doc1, ScoreDoc doc2);
    int compareImplUnknown(ScoreDoc doc1, ScoreDoc doc2);
    int compareImplCustomRanking(ScoreDoc doc1, ScoreDoc doc2);
};

}

#endif

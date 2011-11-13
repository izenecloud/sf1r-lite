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
#include "PropertyData.h"
#include "ScoreDoc.h"
#include "CustomRanker.h"

#include <boost/shared_ptr.hpp>

namespace sf1r{

class CustomRanker;

class SortPropertyComparator
{
public:
    int compare(ScoreDoc doc1, ScoreDoc doc2);

private:
    boost::shared_ptr<PropertyData> propertyData_;
    PropertyDataType type_;
    void* data_;
    int (SortPropertyComparator::*comparator_)(ScoreDoc, ScoreDoc);

public:
    SortPropertyComparator();
    SortPropertyComparator(boost::shared_ptr<PropertyData> propData);
    SortPropertyComparator(PropertyDataType dataType);

private:
    void initComparator();
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

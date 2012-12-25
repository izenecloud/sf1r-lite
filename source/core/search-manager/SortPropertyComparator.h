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

#include "ScoreDoc.h"
#include "CustomRanker.h"

#include <boost/shared_ptr.hpp>

namespace sf1r
{

class SortPropertyComparator
{
public:
    int compare(const ScoreDoc& doc1, const ScoreDoc& doc2) const;

private:
    boost::shared_ptr<NumericPropertyTableBase> propertyTable_;
    NumericPropertyTableBase::MutexType * propertyTableLock_;
    PropertyDataType type_;
    size_t size_;
    int (SortPropertyComparator::*comparator_)(const ScoreDoc& doc1, const ScoreDoc& doc2) const;

public:
    SortPropertyComparator();
    explicit SortPropertyComparator(const boost::shared_ptr<NumericPropertyTableBase>& propData);
    explicit SortPropertyComparator(PropertyDataType dataType);
    ~SortPropertyComparator();
private:
    void initComparator();
    int compareImplDefault(const ScoreDoc& doc1, const ScoreDoc& doc2) const;
    int compareImplNumeric(const ScoreDoc& doc1, const ScoreDoc& doc2) const;
    int compareImplDouble(const ScoreDoc& doc1, const ScoreDoc& doc2) const;
    int compareImplUnknown(const ScoreDoc& doc1, const ScoreDoc& doc2) const;
    int compareImplCustomRanking(const ScoreDoc& doc1, const ScoreDoc& doc2) const;
};

}

#endif

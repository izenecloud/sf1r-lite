/**
 * @file sf1r/search-manager/SortPropertyComparator.h
 * @author Yingfeng Zhang
 * @date Created <2009-10-10>
 * @brief WildcardDocumentIterator DocumentIterator for wildcard query
 */
#ifndef SORT_PROPERTY_COMPARATOR_H
#define SORT_PROPERTY_COMPARATOR_H

#include <common/type_defs.h>
#include "ScoreDoc.h"

namespace sf1r{

class SortPropertyComparator
{
public:
    SortPropertyComparator():type_(UNKNOWN_DATA_PROPERTY_TYPE){}
    SortPropertyComparator(void* data, PropertyDataType type):data_(data), type_(type){}
    ~SortPropertyComparator() {}
public:
    int compare(ScoreDoc doc1, ScoreDoc doc2)
    {
        switch(type_)
        {
        case INT_PROPERTY_TYPE:
            {
                int64_t f1 = ((int64_t*)data_)[doc1.docId];
                int64_t f2 = ((int64_t*)data_)[doc2.docId];
                if (f1 < f2) return -1;
                if (f1 > f2) return 1;
                return 0;
            }
            break;
        case UNSIGNED_INT_PROPERTY_TYPE:
            {
                uint64_t f1 = ((uint64_t*)data_)[doc1.docId];
                uint64_t f2 = ((uint64_t*)data_)[doc2.docId];
                if (f1 < f2) return -1;
                if (f1 > f2) return 1;
                return 0;
            }
            break;
        case FLOAT_PROPERTY_TYPE:
            {
                float f1 = ((float*)data_)[doc1.docId];
                float f2 = ((float*)data_)[doc2.docId];
                if (f1 < f2) return -1;
                if (f1 > f2) return 1;
                return 0;
            }
            break;
        case DOUBLE_PROPERTY_TYPE:
            {
                double f1 = ((double*)data_)[doc1.docId];
                double f2 = ((double*)data_)[doc2.docId];
                if (f1 < f2) return -1;
                if (f1 > f2) return 1;
                return 0;
            }
            break;
        case UNKNOWN_DATA_PROPERTY_TYPE:
            {
                if (doc1.score < doc2.score) return -1;
                if (doc1.score > doc2.score) return 1;
                return 0;
            }
            break;
        default:
            break;
        }
        return 0;
    }
private:
    void* data_;
    PropertyDataType type_;
};

}

#endif

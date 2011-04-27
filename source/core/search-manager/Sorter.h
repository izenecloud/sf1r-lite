/**
 * @file sf1r/search-manager/Sorter.h
 * @author Yingfeng Zhang
 * @date Created <2009-10-10>
 * @brief Providing utilities for sorting documents according to properties
 */
#ifndef _SORTER_H
#define _SORTER_H

#include <index-manager/IndexManager.h>
#include "SortPropertyComparator.h"

#include <boost/thread.hpp>

#include <list>
#include <map>
#include <string>

using namespace std;

namespace sf1r{
/*
* @brief Sortproperty  indicates a property according to which the query results are sorted
*/
class SortProperty
{
public:
    enum SortPropertyType
    {
        SCORE, ///sort by ranking score
        AUTO,   ///sort by ranking score and property
        CUSTOM  ///sort by customized comparator
    };
public:
    SortProperty(const SortProperty& src);

    SortProperty(const string& property, PropertyDataType propertyType, bool reverse = false);

    SortProperty(const string& property, PropertyDataType propertyType, SortPropertyType type, bool reverse = false);
    ///pComparator is generated outside by customer
    SortProperty(const string& property, PropertyDataType propertyType, SortPropertyComparator* pComparator, bool reverse = false);

    ~SortProperty();

public:
    string& getProperty() { return property_; }

    PropertyDataType getPropertyDataType() { return propertyDataType_;}

    SortPropertyType getType() { return type_; }

    bool isReverse() { return reverse_;}

    SortPropertyComparator* getComparator() { return pComparator_; }

private:
    ///name of property to be sorted 
    string property_;
    ///type of data of this sorted proeprty
    PropertyDataType propertyDataType_;

    SortPropertyType type_;
    ///whether order results ASC or DESC
    bool reverse_;
    ///practical comparator
    SortPropertyComparator* pComparator_;

    friend class Sorter;
};

/*
* @brief SortPropertyCache, load data for property to be sorted
* it will be reloaded if index has been changed
*/
class SortPropertyCache
{
public:
    SortPropertyCache(IndexManager* pIndexer);

    ~SortPropertyCache();
public:
    ///If index has been changed, we should reload
    void setDirty(bool dirty) { dirty_ = dirty;}

    SortPropertyComparator* getComparator(SortProperty* pSortProperty);

    bool getSortPropertyData(const std::string& propertyName, PropertyDataType propertyType, void* &data);

private:
    void loadSortData(const std::string& property, PropertyDataType type);

    void clear();

    void clear(const std::string& property);
private:
    ///All data would be got through IndexManager
    IndexManager* pIndexer_;

    bool dirty_;

    ///key: the name of sorted property
    ///value: memory pool containing the property data 
    ///We use void* to store the data, therefore PropertyDataType is required to be
    ///specified for malloc/free the memory
    std::map<std::string, std::pair<PropertyDataType,void*> > sortDataCache_;

    boost::mutex mutex_;	
};

/*
* @brief Sorter main interface exposed. 
*/
class Sorter
{
public:
    Sorter(SortPropertyCache* pCache);

    ~Sorter();

public:

    void addSortProperty(const string& property, PropertyDataType propertyType, bool reverse = false);

    void addSortProperty(SortProperty* pSortProperty);

    inline bool lessThan(ScoreDoc doc1,ScoreDoc doc2);

    ///This interface would be called after an instance of Sorter is established, it will generate SortPropertyComparator
    /// for internal usage
    void getComparators();

private:
    SortPropertyCache* pCache_;

    std::list<SortProperty*> sortProperties_;

    SortProperty** ppSortProperties_;

    size_t nNumProperties_;
};

inline bool Sorter::lessThan(ScoreDoc doc1,ScoreDoc doc2)
{
    int c = 0;
    size_t i=0; 
    SortProperty* pSortProperty;
    while((i<nNumProperties_) &&(c==0))
    {
        pSortProperty = ppSortProperties_[i];
        c = (pSortProperty->isReverse()) ? pSortProperty->pComparator_->compare (doc2, doc1) 
               : pSortProperty->pComparator_->compare (doc1, doc2);
        i++;
    }

    return c < 0;
}

}

#endif

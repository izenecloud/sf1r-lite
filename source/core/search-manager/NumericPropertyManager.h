/**
 * @file sf1r/search-manager/NumericPropertyManager.h
 * @author August Njam Grong <ran.long@izenesoft.com>
 * @date Created <2011-08-31>
 * @brief Providing an interface to retrieve property values from cache.
 */
#ifndef NUMERIC_PROPERTY_MANAGER_H
#define NUMERIC_PROPERTY_MANAGER_H

#include "Sorter.h"

using namespace std;

namespace sf1r{

class NumericPropertyTable
{
public:
    NumericPropertyTable(PropertyDataType type, void *data): type_(type), data_(data)
    {}

    inline bool getIntPropertyValue(const docid_t did, int64_t &value)
    {
        if (type_ != INT_PROPERTY_TYPE)
            return false;

        value = ((int64_t *)data_)[did];
        return true;
    }

    inline bool getUnsignedPropertyValue(const docid_t did, uint64_t &value)
    {
        if (type_ != UNSIGNED_INT_PROPERTY_TYPE)
            return false;

        value = ((uint64_t *)data_)[did];
        return true;
    }

    inline bool getFloatPropertyValue(const docid_t did, float &value)
    {
        if (type_ != FLOAT_PROPERTY_TYPE)
            return false;

        value = ((float *)data_)[did];
        return true;
    }

    inline bool getDoublePropertyValue(const docid_t did, double &value)
    {
        if (type_ != DOUBLE_PROPERTY_TYPE)
            return false;

        value = ((double *)data_)[did];
        return true;
    }

private:
    PropertyDataType type_;

    void *data_;
};

class NumericPropertyManager
{
public:
    static NumericPropertyManager* instance();

    void setPropertyCache(SortPropertyCache *propertyCache);

    NumericPropertyTable* getPropertyTable(const string& property, PropertyDataType type);

private:
    SortPropertyCache *propertyCache_;

    static NumericPropertyManager *instance_;
};

}

#endif

/**
 * @file sf1r/search-manager/NumericPropertyTable.h
 * @author August Njam Grong <ran.long@izenesoft.com>
 * @date Created <2011-08-31>
 * @brief Providing an interface to retrieve property values from cache.
 */
#ifndef NUMERIC_PROPERTY_TABLE_H
#define NUMERIC_PROPERTY_TABLE_H

#include <common/type_defs.h>

using namespace std;

namespace sf1r{

class NumericPropertyTable
{
public:
    NumericPropertyTable(PropertyDataType type, void *data): type_(type), data_(data)
    {}

    PropertyDataType getType() const
    {
        return type_;
    }

    inline bool getIntPropertyValue(const docid_t did, int64_t &value) const
    {
        if (type_ != INT_PROPERTY_TYPE)
            return false;

        value = ((int64_t *)data_)[did];
        return true;
    }

    inline bool getUnsignedPropertyValue(const docid_t did, uint64_t &value) const
    {
        if (type_ != UNSIGNED_INT_PROPERTY_TYPE)
            return false;

        value = ((uint64_t *)data_)[did];
        return true;
    }

    inline bool getFloatPropertyValue(const docid_t did, float &value) const
    {
        if (type_ != FLOAT_PROPERTY_TYPE)
            return false;

        value = ((float *)data_)[did];
        return true;
    }

    inline bool getDoublePropertyValue(const docid_t did, double &value) const
    {
        if (type_ != DOUBLE_PROPERTY_TYPE)
            return false;

        value = ((double *)data_)[did];
        return true;
    }

    template<typename T>
    inline void getPropertyValue(const docid_t did, T &value) const
    {
        value = ((T *)data_)[did];
    }

private:
    PropertyDataType type_;

    void *data_;
};

}

#endif

/**
 * @file sf1r/search-manager/NumericPropertyTable.h
 * @author August Njam Grong <ran.long@izenesoft.com>
 * @date Created <2011-08-31>
 * @brief Providing an interface to retrieve property values from cache.
 */
#ifndef NUMERIC_PROPERTY_TABLE_H
#define NUMERIC_PROPERTY_TABLE_H

#include <common/type_defs.h>

#include <string>

namespace sf1r{

class NumericPropertyTable
{
public:
    NumericPropertyTable(const std::string &name, PropertyDataType type, void *data): name_(name), type_(type), data_(data)
    {}

    const std::string &getPropertyName() const
    {
        return name_;
    }

    PropertyDataType getPropertyType() const
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

    inline int64_t getIntPropertyValue(const docid_t did) const
    {
        return ((int64_t *)data_)[did];
    }

    inline bool getUnsignedPropertyValue(const docid_t did, uint64_t &value) const
    {
        if (type_ != UNSIGNED_INT_PROPERTY_TYPE)
            return false;

        value = ((uint64_t *)data_)[did];
        return true;
    }

    inline uint64_t getUnsignedPropertyValue(const docid_t did) const
    {
        return ((uint64_t *)data_)[did];
    }

    inline bool getFloatPropertyValue(const docid_t did, float &value) const
    {
        if (type_ != FLOAT_PROPERTY_TYPE)
            return false;

        value = ((float *)data_)[did];
        return true;
    }

    inline float getFloatPropertyValue(const docid_t did) const
    {
        return ((float *)data_)[did];
    }

    inline bool getDoublePropertyValue(const docid_t did, double &value) const
    {
        if (type_ != DOUBLE_PROPERTY_TYPE)
            return false;

        value = ((double *)data_)[did];
        return true;
    }

    inline double getDoublePropertyValue(const docid_t did) const
    {
        return ((double *)data_)[did];
    }

    template<typename T>
    inline void getPropertyValue(const docid_t did, T &value) const
    {
        value = ((T *)data_)[did];
    }

private:
    std::string name_;

    PropertyDataType type_;

    void *data_;
};

}

#endif

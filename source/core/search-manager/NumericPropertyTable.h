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
    NumericPropertyTable(const std::string &name, PropertyDataType type, void *data)
        : name_(name)
        , type_(type)
        , data_(data)
    {}

    const std::string &getPropertyName() const
    {
        return name_;
    }

    PropertyDataType getPropertyType() const
    {
        return type_;
    }

    template <typename T>
    inline bool convertPropertyValue(const docid_t did, T &value) const
    {
        switch(type_)
        {
        case INT_PROPERTY_TYPE:
            value = ((int64_t *)data_)[did];
            break;

        case UNSIGNED_INT_PROPERTY_TYPE:
            value = ((uint64_t *)data_)[did];
            break;

        case FLOAT_PROPERTY_TYPE:
            value = ((float *)data_)[did];
            break;

        case DOUBLE_PROPERTY_TYPE:
            value = ((double *)data_)[did];
            break;

        default:
            return false;
        }

        return true;
    }

private:
    std::string name_;

    PropertyDataType type_;

    void *data_;
};

}

#endif

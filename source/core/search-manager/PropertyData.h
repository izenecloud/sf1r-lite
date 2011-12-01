/**
 * @file PropertyData.h
 * @author Jun Jiang
 * @date Created <2011-11-10>
 * @brief for a specific property type, it stores each property value for each doc
 */
#ifndef PROPERTY_DATA_H
#define PROPERTY_DATA_H

#include <common/type_defs.h>

namespace sf1r{

struct PropertyData
{
    PropertyDataType type_;
    void* data_;
    time_t lastLoadTime_;

    PropertyData(PropertyDataType type, void* data)
        : type_(type), data_(data)
    {
        resetLoadTime();
    }

    void resetLoadTime()
    {
        lastLoadTime_ = std::time(NULL);
    }

    time_t elapsedFromLastLoad()
    {
        return (std::time(NULL) - lastLoadTime_);
    }

    ~PropertyData()
    {
        switch (type_)
        {
        case INT_PROPERTY_TYPE:
            delete[] (int64_t*)data_;
            break;

        case UNSIGNED_INT_PROPERTY_TYPE:
            delete[] (uint64_t*)data_;
            break;

        case FLOAT_PROPERTY_TYPE:
            delete[] (float*)data_;
            break;

        case DOUBLE_PROPERTY_TYPE:
            delete[] (double*)data_;
            break;

        default:
            break;
        }
    }
};

} // namespace sf1r

#endif // PROPERTY_DATA_H

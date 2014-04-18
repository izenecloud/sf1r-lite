#ifndef SF1V5_DOCUMENT_MANAGER_PROPERTY_VALUE_H
#define SF1V5_DOCUMENT_MANAGER_PROPERTY_VALUE_H
/**
 * @file sf1r/document-manager/PropertyValue.h
 * @date Created <2009-10-21 18:15:33>
 * @date Updated <2009-10-22 03:46:01>
 */
#include "type_defs.h"
#include <sf1common/PropertyValue.h>

namespace sf1r {

using izenelib::PropertyValue;
template<typename T>
inline T& get(PropertyValue& p)
{
    return izenelib::get<T>(p);
}
template<typename T>
inline const T& get(const PropertyValue& p)
{
    return izenelib::get<T>(p);
}
template<typename T>
inline T* get(PropertyValue* p)
{
    return izenelib::get<T>(p);
}
template<typename T>
inline const T* get(const PropertyValue* p)
{
    return izenelib::get<T>(p);
}

inline void swap(PropertyValue& a, PropertyValue& b)
{
    izenelib::swap(a, b);
}

inline bool operator==(const PropertyValue& a,
                       const PropertyValue& b)
{
    return izenelib::operator==(a, b);
}
inline bool operator!=(const PropertyValue& a,
                       const PropertyValue& b)
{
    return izenelib::operator!=(a, b);
}

} // end namespace sf1r


#endif // SF1V5_DOCUMENT_MANAGER_PROPERTY_VALUE_H

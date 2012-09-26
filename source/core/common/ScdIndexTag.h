/*
 * File:   ScdIndexTag.h
 * Author: Paolo D'Apice
 *
 * Created on September 17, 2012, 1:55 PM
 */

#ifndef SCDINDEXTAG_H
#define SCDINDEXTAG_H

#include "uint128.h"
#include "ScdParserTraits.h"
#include <boost/mpl/int.hpp>

namespace scd {

/// Conversion function object declaration.
template <typename Type> struct Converter;

/// Default implementation as the identity function.
template <>
struct Converter<PropertyValueType> {
    inline PropertyValueType operator()(const PropertyValueType& in) const {
        return in;
    }
};

/// Base tag type.
template <class Derived, typename Type = PropertyValueType, typename TypeConverter = Converter<Type> >
struct TagType {
    typedef Type type;

    /// Convert the parsed value.
    static inline Type convert(const PropertyValueType& parsed) {
        static TypeConverter converter;
        return converter(parsed);
    }

    /// @return The tag name.
    static inline const char* name() {
        return Derived::name();
    }
};

} /* namespace scd */

/**
 * @brief Create a new property tag.
 */
#define SCD_INDEX_PROPERTY_TAG(tag_name) \
    struct tag_name : scd::TagType<tag_name> { \
        static inline const char* name() { return #tag_name; } \
        typedef boost::mpl::int_< sizeof(#tag_name) >::type hash_type; \
    }

/**
 * @brief Create a new property tag with a specific type.
 *
 * A conversion function object in the scd namespace is needed in order to
 * properly convert the PropertyValueType value (returned by the parser).
 */
#define SCD_INDEX_PROPERTY_TAG_TYPED(tag_name, tag_type) \
    struct tag_name : scd::TagType<tag_name, tag_type> { \
        static inline const char* name() { return #tag_name; } \
        typedef boost::mpl::int_< sizeof(#tag_name) >::type hash_type; \
    }

namespace scd {

/// Default tag for DOCID property.
SCD_INDEX_PROPERTY_TAG_TYPED(DOCID, uint128);

/// Converter specialization for the uint128.
template<>
struct Converter<uint128> {
    inline uint128 operator()(const PropertyValueType& in) const {
        return str2uint(in);
    }
};

} /* namespace scd */

#endif /* SCDINDEXTAG_H */

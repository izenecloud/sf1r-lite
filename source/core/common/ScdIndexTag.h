/*
 * File:   ScdIndexTag.h
 * Author: Paolo D'Apice
 *
 * Created on September 17, 2012, 1:55 PM
 */

#ifndef SCDINDEXTAG_H
#define SCDINDEXTAG_H

#include "ScdParserTraits.h"

namespace scd {

/// Conversion function object declaration.
template <typename Type> struct Converter;

/// Converter specialization for the default type std::string.
template <> struct Converter<std::string> {
    inline std::string operator()(const PropertyValueType& in) const {
        std::string str;
        in.convertString(str, izenelib::util::UString::UTF_8);
        return str;
    }
};

/// Base tag type.
template <class Derived, typename Type = std::string, typename TypeConverter = Converter<Type> >
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
    }

namespace scd {

SCD_INDEX_PROPERTY_TAG(DOCID); //< Default tag for 'DOCID' property.
SCD_INDEX_PROPERTY_TAG(uuid);  //< Default tag for 'uuid' property.

} /* namespace scd */

#endif /* SCDINDEXTAG_H */

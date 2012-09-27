/*
 * File:   ScdIndexTag.h
 * Author: Paolo D'Apice
 *
 * Created on September 17, 2012, 1:55 PM
 */

#ifndef SCDINDEXTAG_H
#define SCDINDEXTAG_H

#include "ScdParserTraits.h"
#include "Utilities.h"

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
 * @brief Create a new property tag using default type.
 */
#define SCD_INDEX_PROPERTY_TAG(tag_name) \
    struct tag_name : scd::TagType<tag_name> { \
        static inline const char* name() { return #tag_name; } \
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
    }

namespace scd {

SCD_INDEX_PROPERTY_TAG_TYPED(DOCID, uint128_t); //< Default tag for 'DOCID' property.
SCD_INDEX_PROPERTY_TAG_TYPED(uuid, uint128_t);  //< Default tag for 'uuid' property.

/// Converter specialization for the uint128_t.
template<> struct Converter<uint128_t> {
    inline uint128_t operator()(const PropertyValueType& in) const {
        return sf1r::Utilities::md5ToUint128(in);
    }
};

} /* namespace scd */

// operator<< should be in the global namespace
std::ostream& operator<<(std::ostream& os, const uint128_t& in) {
    os << sf1r::Utilities::uint128ToMD5(in);
    return os;
}

#endif /* SCDINDEXTAG_H */

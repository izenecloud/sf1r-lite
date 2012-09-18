/*
 * File:   ScdIndexTag.h
 * Author: Paolo D'Apice
 *
 * Created on September 17, 2012, 1:55 PM
 */

#ifndef SCDINDEXTAG_H
#define SCDINDEXTAG_H

#include "uint128.h"
#include <util/ustring/UString.h>

// TODO: sostuire le macro ed usare funzioni globali

/**
 * @brief Create a new property tag.
 * This tag will hold a value with type izenelib::util::UString&
 * (the default type returned by the parser).
 */
#define SCD_INDEX_PROPERTY_TAG(tag_name) \
    struct tag_name { \
        typedef izenelib::util::UString type; \
        static inline izenelib::util::UString convert(const izenelib::util::UString& parsed) { \
            return parsed; \
        } \
        static const char name[]; \
        static const int hash = sizeof(#tag_name); \
    }; \
    const char tag_name::name[] = #tag_name

/**
 * @brief Create a new property tag with a specific type.
 * This tag will hold a value with the specified type.
 * A conversion function is needed in order to get its value from
 * izenelib::util::UString& (the default type returned by the parser).
 */
#define SCD_INDEX_PROPERTY_TAG_TYPED(tag_name, tag_type, type_converter) \
    struct tag_name { \
        typedef tag_type type; \
        static inline tag_type convert(const izenelib::util::UString& parsed) { \
            return type_converter(parsed); \
        } \
        static const char name[]; \
        static const int hash = sizeof(#tag_name); \
    }; \
    const char tag_name::name[] = #tag_name

namespace scd {

/// Default tag for DOCID property.
SCD_INDEX_PROPERTY_TAG_TYPED(DOCID, uint128, str2uint);

} /* namespace scd */

#endif /* SCDINDEXTAG_H */

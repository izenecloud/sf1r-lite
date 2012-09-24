/*
 * File:   ScdIndexLeveldbTags.h
 * Author: Paolo D'Apice
 *
 * Created on September 24, 2012, 3:20 PM
 */

#ifndef SCDINDEX_LEVELDB_TAGS_H
#define SCDINDEX_LEVELDB_TAGS_H

#include "ScdIndexTag.h"

namespace scd {

/// Converter specialization for the std::string
template<>
struct Converter<std::string> {
    inline std::string operator()(const PropertyValueType& in) const {
        std::string str;
        in.convertString(str, izenelib::util::UString::UTF_8);
        return str;
    }
};

namespace leveldb {

SCD_INDEX_PROPERTY_TAG_TYPED(DOCID, std::string); //< Default tag for 'DOCID' property.
SCD_INDEX_PROPERTY_TAG_TYPED(uuid, std::string);  //< Default tag for 'uuid' property.

}} /* namespace scd::leveldb */

#endif /* SCDINDEX_LEVELDB_TAGS_H */

#ifndef _SF1R_TYPE_DEFINITIONS_H_
#define _SF1R_TYPE_DEFINITIONS_H_

#include <sf1common/type_defs.h>
#include "inttypes.h"

namespace sf1r
{
using izenelib::PropertyDataType;
using izenelib::STRING_PROPERTY_TYPE;

using izenelib::INT8_PROPERTY_TYPE;
using    izenelib::INT16_PROPERTY_TYPE;
using    izenelib::INT32_PROPERTY_TYPE;
using    izenelib::INT64_PROPERTY_TYPE;

using    izenelib::FLOAT_PROPERTY_TYPE;
using    izenelib::DOUBLE_PROPERTY_TYPE;

using    izenelib::DATETIME_PROPERTY_TYPE;

using    izenelib::NOMINAL_PROPERTY_TYPE;
using    izenelib::UNKNOWN_DATA_PROPERTY_TYPE;
    // document-manager's  internal data types
using    izenelib::FORWARD_INDEX_PROPERTY_TYPE;

using    izenelib::CBIT_ARRAY_PROPERTY_TYPE;
using    izenelib::USTRING_PROPERTY_TYPE;
using    izenelib::USTRING_ARRAY_PROPERTY_TYPE;
using    izenelib::VECTOR_UNSIGNED_INT_TYPE;
using    izenelib::VECTOR_VECTOR_UNSIGNED_INT_TYPE;
    //customized type
using    izenelib::CUSTOM_RANKING_PROPERTY_TYPE;
    //subdocument type
using    izenelib::SUBDOC_PROPERTY_TYPE;
using    izenelib::GEOLOCATION_PROPERTY_TYPE;

using izenelib::ID_FREQ_ORDERED_MAP_T;
using izenelib::ID_FREQ_UNORDERED_MAP_T;
using izenelib::ID_FREQ_MAP_T;

using izenelib::INTEGER_LIST_T;
using izenelib::ID_INTLIST_MAP_T;
using izenelib::INT_SET_T;

using izenelib::DocumentFrequencyInProperties;
using izenelib::CollectionTermFrequencyInProperties;
using izenelib::MaxTermFrequencyInProperties;
using izenelib::UpperBoundInProperties;

using izenelib::Positions;
using izenelib::PositionPtr;

using izenelib::offsets;
using izenelib::offsetsPtr;

using izenelib::string_size_pair_t;
using izenelib::id2name_t;

using izenelib::WCMAP_T; // wild card map type
using izenelib::QTISM_T; // query term id set map type

#define SF1R_ENSURE_INIT(condition)                             \
if (! (condition))                                              \
{                                                               \
    std::cerr << "Initialization assertion failed: " #condition \
              << std::endl;                                     \
    return false;                                               \
}

} // end - namespace

#endif  //_SF1R_TYPE_DEFINITIONS_H_

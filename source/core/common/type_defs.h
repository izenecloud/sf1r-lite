/**
 * @file    type_defs.h
 * @brief   defines basic types that are used among the modules in sf1r
 * @author  MyungHyun Lee (Kent)
 * @date    2008-12-30
 * @details
 *  - Log
 *      - 2008.02.05 : Added ID related types which is used in ranking-manager and document-manager, by Dohyun Yun
 */


#ifndef _SF1V5_TYPE_DEFINITIONS_H_
#define _SF1V5_TYPE_DEFINITIONS_H_

#include <util/ustring/UString.h>

#include <util/izene_serialization.h>
#include <am/3rdparty/rde_hash.h>
#include <3rdparty/am/rde_hashmap/hash_map.h>

#include <boost/unordered_map.hpp>
#include <boost/shared_ptr.hpp>

#include <map>
#include <set>
#include <list>

#include "inttypes.h"

namespace sf1r
{
    // TODO: The internal property types are merged with the actual data types of the properties.
    // Do we need all these data types?
    // <<<< @by MyungHyun - Mar 18, 2009
    enum PropertyDataType
    {
        STRING_PROPERTY_TYPE = 0,
        INT_PROPERTY_TYPE,
        FLOAT_PROPERTY_TYPE,
        NOMINAL_PROPERTY_TYPE,
        UNKNOWN_DATA_PROPERTY_TYPE,
        // document-manager's  internal data types
        //FLOAT_TYPE = 0,
        //INT_TYPE,
        UNSIGNED_INT_PROPERTY_TYPE,
        DOUBLE_PROPERTY_TYPE,
        FORWARD_INDEX_PROPERTY_TYPE,
        CBIT_ARRAY_PROPERTY_TYPE,
        //STRING_TYPE,
        USTRING_PROPERTY_TYPE,
        USTRING_ARRAY_PROPERTY_TYPE,
        VECTOR_UNSIGNED_INT_TYPE,
        VECTOR_VECTOR_UNSIGNED_INT_TYPE
        //UNKNOWN_DATA_TYPE
    };

    typedef std::map<unsigned int, float>            ID_FREQ_ORDERED_MAP_T;
    typedef boost::unordered_map<unsigned int, float> ID_FREQ_UNORDERED_MAP_T;
    typedef ID_FREQ_UNORDERED_MAP_T                  ID_FREQ_MAP_T;

    typedef std::list<unsigned int>               INTEGER_LIST_T;
    typedef std::map<unsigned int,INTEGER_LIST_T> ID_INTLIST_MAP_T;
    typedef std::set<unsigned int>                INT_SET_T;

    typedef std::map<std::string, ID_FREQ_MAP_T > DocumentFrequencyInProperties;
    typedef std::map<std::string, ID_FREQ_MAP_T > CollectionTermFrequencyInProperties;

    typedef std::deque<std::pair<unsigned, unsigned> > Positions;
    typedef boost::shared_ptr<const Positions> PositionPtr;

    typedef std::deque<loc_t> offsets;
    typedef boost::shared_ptr<const offsets> offsetsPtr;

    typedef std::pair<std::string , size_t> string_size_pair_t;

    typedef izenelib::am::rde_hash<termid_t,izenelib::util::UString> WCMAP_T; // wild card map type
    typedef boost::unordered_map<std::string , std::set<termid_t> > QTISM_T; // query term id set map type

} // end - namespace

#endif  //_SF1V5_TYPE_DEFINITIONS_H_


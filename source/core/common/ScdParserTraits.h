/*
 * File:   ScdParserTraits.h
 * Author: Paolo D'Apice
 *
 * Created on September 15, 2012, 06:52 PM
 */

#ifndef SCD_PARSER_TRAITS_H
#define SCD_PARSER_TRAITS_H

#include <util/ustring/UString.h>

// We use std::string as propertyName, UString as propertyValue.
typedef std::string PropertyNameType;
//typedef izenelib::util::UString PropertyValueType;
typedef std::string ScdPropertyValueType;

typedef std::pair<PropertyNameType, ScdPropertyValueType> FieldPair;
typedef std::vector<FieldPair> SCDDoc;
typedef boost::shared_ptr<SCDDoc> SCDDocPtr;

typedef long offset_type; //< Type of offset values.
typedef std::vector<offset_type> offset_list;

typedef std::pair<ScdPropertyValueType, unsigned> DocIdPair; // should this be long instead of unsigned?

#endif /* SCD_PARSER_TRAITS_H */

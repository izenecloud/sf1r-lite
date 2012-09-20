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
typedef izenelib::util::UString PropertyValueType;

typedef std::pair<PropertyNameType, PropertyValueType> FieldPair;
typedef std::vector<FieldPair> SCDDoc;
typedef boost::shared_ptr<SCDDoc> SCDDocPtr;

typedef long offset_type; //< Type of offset values.

typedef std::pair<izenelib::util::UString, unsigned> DocIdPair; // should this be long instead of unsigned?

#endif /* SCD_PARSER_TRAITS_H */

/*
 * File:   ScdParserTraits.h
 * Author: Paolo D'Apice
 *
 * Created on September 15, 2012, 06:52 PM
 */

#ifndef SCD_PARSER_TRAITS_H
#define SCD_PARSER_TRAITS_H

#include <sf1common/ScdParserTraits.h>

// We use std::string as propertyName, UString as propertyValue.
using izenelib::PropertyNameType;
//typedef izenelib::util::UString PropertyValueType;
using izenelib::ScdPropertyValueType;

using izenelib::FieldPair;
using izenelib::SCDDoc;
using izenelib::SCDDocPtr;

using izenelib::offset_type; //< Type of offset values.
using izenelib::offset_list;

using izenelib::DocIdPair; // should this be long instead of unsigned?

#endif /* SCD_PARSER_TRAITS_H */

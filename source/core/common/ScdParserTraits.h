/*
 * File:   ScdParserTraits.h
 * Author: Paolo D'Apice
 *
 * Created on September 15, 2012, 06:52 PM
 */

#ifndef SCD_PARSER_TRAITS_H
#define SCD_PARSER_TRAITS_H

#include <util/ustring/UString.h>

///We use std::string as propertyName, UString as propertyValue right now
typedef std::pair<std::string, izenelib::util::UString> FieldPair;
typedef std::vector<FieldPair> SCDDoc;
typedef boost::shared_ptr<SCDDoc> SCDDocPtr;
typedef std::pair<izenelib::util::UString, unsigned> DocIdPair;

#endif /* SCD_PARSER_TRAITS_H */

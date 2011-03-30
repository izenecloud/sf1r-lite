///
/// @file ontology_parser.h
/// @brief ontology xml parser
/// @author Jia Guo <guojia@gmail.com>
/// @date Created 2010-12-21
///

#ifndef SF1R_ONTOLOGY_PARSER_H_
#define SF1R_ONTOLOGY_PARSER_H_

#include <boost/serialization/access.hpp>
#include <boost/serialization/serialization.hpp>
#include <common/type_defs.h>
#include <common/ResultType.h>
#include "faceted_types.h"
NS_FACETED_BEGIN


class OntologyParser
{

public:
    static bool Parse(Ontology* ontology, const std::string& xml);


public:
    std::vector<izenelib::util::UString> labels;
    izenelib::util::UString description;



};
NS_FACETED_END
#endif

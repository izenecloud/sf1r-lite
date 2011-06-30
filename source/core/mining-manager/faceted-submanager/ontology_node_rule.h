///
/// @file ontology_node_rule.h
/// @brief ontology node value
/// @author Jia Guo <guojia@gmail.com>
/// @date Created 2010-12-21
///

#ifndef SF1R_ONTOLOGY_NODERULE_H_
#define SF1R_ONTOLOGY_NODERULE_H_

#include <boost/serialization/access.hpp>
#include <boost/serialization/serialization.hpp>
#include <common/type_defs.h>
#include <common/ResultType.h>
#include "faceted_types.h"
NS_FACETED_BEGIN


class OntologyNodeRule
{

public:
    OntologyNodeRule():labels()
    {
    }

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & labels & description;
    }


public:
    std::vector<izenelib::util::UString> labels;
    izenelib::util::UString description;

};

class OntologyNodeRuleInt
{

public:
     
    

public:
    std::vector<izenelib::util::UString> labels;
    std::vector<std::vector<uint32_t> > labels_int;


};

NS_FACETED_END
#endif

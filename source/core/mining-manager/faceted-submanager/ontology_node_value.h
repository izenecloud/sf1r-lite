///
/// @file ontology_node_value.h
/// @brief ontology node value
/// @author Jia Guo <guojia@gmail.com>
/// @date Created 2010-12-09
///

#ifndef SF1R_ONTOLOGY_NODEVALUE_H_
#define SF1R_ONTOLOGY_NODEVALUE_H_

#include <boost/serialization/access.hpp>
#include <boost/serialization/serialization.hpp>
#include <common/type_defs.h>
#include <common/ResultType.h>
#include "faceted_types.h"
#include "ontology_node_rule.h"
NS_FACETED_BEGIN


class OntologyNodeValue {

typedef std::list<CategoryIdType> ChildrenType;
public:
  OntologyNodeValue():children(), parent_id(0), name()
  {
  }
  
  OntologyNodeValue(CategoryIdType par_parent_id, const CategoryNameType& par_name):children(), parent_id(par_parent_id), name(par_name)
  {
  }
  
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, const unsigned int version)
  {
      ar & children & parent_id & name & rule;
  }
  
  
public:
  ChildrenType children;
  CategoryIdType parent_id;
  CategoryNameType name;
  OntologyNodeRule rule;
  
    
};
NS_FACETED_END
#endif 

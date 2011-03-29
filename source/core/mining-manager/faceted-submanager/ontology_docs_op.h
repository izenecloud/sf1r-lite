///
/// @file ontology_docs_op.h
/// @brief ontology docs operations class
/// @author Jia Guo <guojia@gmail.com>
/// @date Created 2010-12-10
///

#ifndef SF1R_ONTOLOGY_DOCS_OP_H_
#define SF1R_ONTOLOGY_DOCS_OP_H_

#include <boost/serialization/access.hpp>
#include <boost/serialization/serialization.hpp>
#include <common/type_defs.h>
#include <common/ResultType.h>
#include "faceted_types.h"
NS_FACETED_BEGIN


class OntologyDocsOp {
public:
  enum OP {INS, DEL};
  
  OP op;
  CategoryIdType cid;
  uint32_t docid;
  
  static bool op_cid_less(const OntologyDocsOp& op1, const OntologyDocsOp& op2)
  {
    return op1.cid<op2.cid;
  }

  static bool op_docid_less(const OntologyDocsOp& op1, const OntologyDocsOp& op2)
  {
    return op1.docid<op2.docid;
  }

  static bool op_docid_greater(const OntologyDocsOp& op1, const OntologyDocsOp& op2)
  {
    return op1.docid>op2.docid;
  }
    
};




NS_FACETED_END
#endif 

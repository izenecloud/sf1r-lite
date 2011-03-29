///
/// @file ontology-searcher.h
/// @brief search handler for ontologies
/// @author Jia Guo <guojia@gmail.com>
/// @date Created 2010-12-13
///

#ifndef SF1R_ONTOLOGY_SEARCHER_H_
#define SF1R_ONTOLOGY_SEARCHER_H_


#include <boost/serialization/access.hpp>
#include <boost/serialization/serialization.hpp>
#include <common/type_defs.h>
#include <common/ResultType.h>
#include "faceted_types.h"
#include "ontology.h"
NS_FACETED_BEGIN


/// @brief The memory representation form of a taxonomy.
class OntologySearcher {
  
struct NodePath {
  NodePath(CategoryIdType pid, CategoryIdType this_id)
  :parent_id(pid), id(this_id), doc_count(1)
  {
  }
  CategoryIdType parent_id;
  CategoryIdType id;
  uint32_t doc_count;
  
  bool operator==(const NodePath& np) const
  {
    return parent_id==np.parent_id&&id==np.id;
  }
  
  static bool Compare(const NodePath& np1, const NodePath& np2)
  {
    if(np1.parent_id==np2.parent_id)
    {
      return np1.id<np2.id;
    }
    else
    {
      return np1.parent_id<np2.parent_id;
    }
  }
  
  static bool CompareCount(const NodePath& np1, const NodePath& np2)
  {
    if(np1.parent_id==np2.parent_id)
    {
      return np1.doc_count>np2.doc_count;
    }
    else
    {
      return np1.parent_id<np2.parent_id;
    }
  }
  
};

public:
typedef Ontology::DocidListType DocidListType;
typedef Ontology::CidListType CidListType;
typedef std::list<NodePath>::iterator ListIterator;
typedef std::pair<ListIterator, ListIterator> ListIteratorPair;
  OntologySearcher();
  OntologySearcher(Ontology* ontology);
  ~OntologySearcher();
  
  bool GetStaticRepresentation(OntologyRep& rep);
  
  bool StaticClick(CategoryIdType id, DocidListType& docid_list);
  
  bool GetRepresentation(const std::vector<uint32_t>& search_result, OntologyRep& rep);
  
  bool Click(const std::vector<uint32_t>& search_result, CategoryIdType id, DocidListType& docid_list);
  
  void SetOntology(Ontology* ontology)
  {
    ontology_ = ontology;
  }
  
  Ontology* GetOntology()
  {
    return ontology_;
  }
    
private:
  
  bool AddToRep_(OntologyRep& rep, std::list<NodePath>& all_nodes, izenelib::am::rde_hash<CategoryIdType, ListIteratorPair>& parent_scope, CategoryIdType parent_id, uint8_t depth);
    
private:
  Ontology* ontology_;
  
    

};
NS_FACETED_END
#endif /* SF1R_ONTOLOGY_REP_ITEM_H_ */

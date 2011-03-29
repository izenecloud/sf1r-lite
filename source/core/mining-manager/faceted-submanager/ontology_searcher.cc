#include "ontology_searcher.h"
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/vector.hpp>

#include <iostream>
#include <fstream>

using namespace sf1r::faceted;

OntologySearcher::OntologySearcher():ontology_(NULL)
{
}

OntologySearcher::OntologySearcher(Ontology* ontology)
:ontology_(ontology)
{
}

OntologySearcher::~OntologySearcher()
{
}

bool OntologySearcher::GetStaticRepresentation(OntologyRep& rep)
{
  if(ontology_==NULL) return false;
  return ontology_->GetRepresentation(rep);
}

bool OntologySearcher::StaticClick(CategoryIdType id, DocidListType& docid_list)
{
  if(ontology_==NULL) return false;
  return ontology_->GetRecursiveDocSupport(id, docid_list);
}

bool OntologySearcher::GetRepresentation(const std::vector<uint32_t>& search_result, OntologyRep& rep)
{
  if(ontology_==NULL) return false;
//   izenelib::am::rde_hash<std::vector<CategoryIdType>, uint32_t> position_map;
  std::list<NodePath> all_nodes;
  std::vector<uint32_t>::const_iterator sr_it = search_result.begin();
  izenelib::am::rde_hash<CategoryIdType, bool> indoc_app;
  
  while(sr_it!= search_result.end() )
  {
    indoc_app.clear();
    uint32_t docid = *sr_it;
    sr_it++;
    CidListType cid_list;
    if( ontology_->GetCategories(docid, cid_list) )
    {
      CidListType::iterator cit = cid_list.begin();
      while(cit!=cid_list.end())
      {
        CategoryIdType id = *cit;
        ++cit;
        while(true)
        {
          if( indoc_app.find(id) ) break;
          else
          {
            indoc_app.insert(id,1);
          } 
          CategoryIdType parent_id;
          if( !ontology_->GetParent(id, parent_id) )
          {
            break;
          }
          NodePath node_path(parent_id, id);
          all_nodes.push_back(node_path);
//           std::cout<<"[NP] "<<parent_id<<","<<id<<std::endl;
          id = parent_id;
        }
      }
    }
  }
  all_nodes.sort(NodePath::Compare);
  std::list<NodePath>::iterator nit = all_nodes.begin();
  while(nit!=all_nodes.end())
  {
    std::list<NodePath>::iterator same_begin(nit);
    ++same_begin;
    std::list<NodePath>::iterator same_end(same_begin);
    while( same_end!= all_nodes.end() && (*nit)==(*same_end) )
    {
      (*nit).doc_count += (*same_end).doc_count;
      ++same_end;
    }
    if( same_end!=same_begin ) nit = all_nodes.erase(same_begin, same_end);
    else ++nit;
  }
  all_nodes.sort(NodePath::CompareCount);
  
  izenelib::am::rde_hash<CategoryIdType, ListIteratorPair> parent_scope;
  nit = all_nodes.begin();
  if( nit == all_nodes.end() )
  {
    std::cout<<"No ontology found"<<std::endl;
    return true;
  }
  CategoryIdType current_parent_id = (*nit).parent_id;
  ListIteratorPair current_it_pair(nit, all_nodes.end());
  ++nit;
  while(nit!=all_nodes.end())
  {
    if( (*nit).parent_id!=current_parent_id )
    {
      current_it_pair.second = nit;
      parent_scope.insert( current_parent_id, current_it_pair);
      current_parent_id = (*nit).parent_id;
      current_it_pair = ListIteratorPair(nit, all_nodes.end());
    }
    ++nit;
  }
  parent_scope.insert( current_parent_id, current_it_pair);
  if( !AddToRep_(rep, all_nodes, parent_scope, Ontology::TopCategoryId(), 1) ) return false;
  return true;
}

bool OntologySearcher::Click(const std::vector<uint32_t>& search_result, CategoryIdType id, DocidListType& docid_list)
{
  if(ontology_==NULL) return false;
  std::vector<uint32_t>::const_iterator sr_it = search_result.begin();
  while(sr_it!= search_result.end() )
  {
    uint32_t docid = *sr_it;
    if( ontology_->DocInCategory(docid, id ) )
    {
      docid_list.push_back(docid);
    }
    sr_it++;
  }
  return true;
}

bool OntologySearcher::AddToRep_(OntologyRep& rep, std::list<NodePath>& all_nodes, izenelib::am::rde_hash<CategoryIdType, ListIteratorPair>& parent_scope, CategoryIdType parent_id, uint8_t depth)
{
  if(ontology_==NULL) return false;
  ListIteratorPair iterator_pair;
  if(!parent_scope.get(parent_id, iterator_pair) ) return true;
  while(iterator_pair.first!=iterator_pair.second)
  {
    //get information
    OntologyRepItem rep_item;
    rep_item.level = depth;
    rep_item.id = (*iterator_pair.first).id;
    rep_item.doc_count = (*iterator_pair.first).doc_count;
    if(!ontology_->GetCategoryName(rep_item.id, rep_item.text) ) return false;
    rep.item_list.push_back(rep_item);
    if(!AddToRep_(rep, all_nodes, parent_scope, rep_item.id, depth+1)) return false;
    ++iterator_pair.first;
  }
  
  return true;
  
}



#include "ontology_nodes.h"
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/vector.hpp>

#include <iostream>
#include <fstream>

using namespace sf1r::faceted;

OntologyNodes::OntologyNodes()
{
}
OntologyNodes::~OntologyNodes()
{
}

bool OntologyNodes::Open(const std::string& file)
{
  file_ = file;
  try
  {
    if( boost::filesystem::exists(file) )
    {
      std::ifstream ifs(file.c_str(), std::ios::binary);
      if( ifs.fail()) return false;
      {
        boost::archive::text_iarchive ia(ifs);
        ia >> available_id_list_ >> nodes_ ;
      }
      ifs.close();
      if( ifs.fail() )
      {
        return false;
      }
    }
  }
  catch(std::exception& ex)
  {
    return false;
  }
  std::list<CategoryIdType>::iterator ait = available_id_list_.begin();
  while( ait!=available_id_list_.end() )
  {
    available_id_map_.insert( *ait );
    ait++;
  }
  //if no node, add Top node in it.
  if( nodes_.empty() )
  {
    OntologyNodeValue top(0, izenelib::util::UString("__top", izenelib::util::UString::UTF_8));
    nodes_.push_back(top);
  }
  return true;
}

// bool OntologyNodes::CopyFrom(const std::string& from_file, const std::string& to_file)
// {
//   try
//   {
//     if(!boost::filesystem::exists(from_file) ) return false;
//     boost::filesystem::copy(from_file, to_file);
//   }
//   catch(std::exception& ex)
//   {
//     return false;
//   }
//   return true;
// }

bool OntologyNodes::AddCategoryNode(CategoryIdType parent_id, const CategoryNameType& name, CategoryIdType& id)
{
  if( !IsValidId(parent_id, true) ) return false;
  if( !available_id_list_.empty() )
  {
    std::list<CategoryIdType>::iterator it = available_id_list_.begin();
    id = (*it);
    available_id_list_.erase(it);
    available_id_map_.erase(id);
    nodes_[id].name = name;
    nodes_[id].parent_id = parent_id;
    nodes_[id].children.clear();
  }
  else
  {
    id = nodes_.size();
    OntologyNodeValue node_value(parent_id, name);
    nodes_.push_back(node_value);
  }
  nodes_[parent_id].children.push_back(id);
  return true;
}

bool OntologyNodes::RenameCategory(CategoryIdType id, const CategoryNameType& new_name)
{
  if( !IsValidId(id, false) ) return false;
  nodes_[id].name = new_name;
  return true;
}

bool OntologyNodes::MoveCategory(CategoryIdType id, CategoryIdType new_parent_id)
{
  if( !IsValidId(id, false) || !IsValidId(new_parent_id, true) ) return false;
  CategoryIdType parent_id;
  if(!GetParent(id, parent_id)) return false;
  nodes_[parent_id].children.remove(id);
  nodes_[new_parent_id].children.push_back(id);
  nodes_[id].parent_id = new_parent_id;
  return true;
}

bool OntologyNodes::DelCategory(CategoryIdType id)
{
  //must be leaf node
  if(!IsValidId(id, false)) return false;
  ChildrenType children;
  if(!GetChildren(id, children) ) return false;
  if( !children.empty()) return false;
  CategoryIdType parent_id;
  if(!GetParent(id, parent_id)) return false;
  nodes_[parent_id].children.remove(id);
  available_id_list_.push_back(id);
  available_id_list_.sort();
  available_id_map_.insert(id);
  return true;
}

bool OntologyNodes::GetChildren(CategoryIdType parent_id, ChildrenType& children) const
{
  if(!IsValidId(parent_id, true)) return false;
  children = nodes_[parent_id].children;
  return true;
}
  
bool OntologyNodes::GetParent(CategoryIdType id, CategoryIdType& parent_id) const
{
  if(!IsValidId(id, false)) return false;
  parent_id = nodes_[id].parent_id;
  return true;
}

bool OntologyNodes::GetCategoryName(CategoryIdType id, CategoryNameType& name) const
{
  if(!IsValidId(id, true)) return false;
  name = nodes_[id].name;
  return true;
}

bool OntologyNodes::GetCategoryRule(CategoryIdType id, OntologyNodeRule& rule) const
{
  if(!IsValidId(id, true)) return false;
  rule = nodes_[id].rule;
  return true;
}

bool OntologyNodes::SetCategoryRule(CategoryIdType id, const OntologyNodeRule& rule)
{
  if(!IsValidId(id, true)) return false;
  nodes_[id].rule = rule;
  return true;
}

bool OntologyNodes::IsValidId(CategoryIdType id, bool valid_top) const
{
  if( !valid_top && id==TopCategoryId() ) return false;
  if( id>= nodes_.size() || available_id_map_.find(id)!=available_id_map_.end() )
  {
    return false;
  }
  return true;
}


bool OntologyNodes::Save()
{
  std::ofstream ofs(file_.c_str());
  if( ofs.fail()) return false;
  {
    boost::archive::text_oarchive oa(ofs);
    oa << available_id_list_ << nodes_ ;
  }
  ofs.close();
  if( ofs.fail() )
  {
    return false;
  }
  return true;
}

bool OntologyNodes::AddCategoryNodeStr(CategoryIdType parent_id, const std::string& name, CategoryIdType& id)
{
  CategoryNameType uname(name, izenelib::util::UString::UTF_8);
  return AddCategoryNode(parent_id, uname, id);
}

bool OntologyNodes::RenameCategoryStr(CategoryIdType id, const std::string& new_name)
{
  CategoryNameType uname(new_name, izenelib::util::UString::UTF_8);
  return RenameCategory(id, uname);
}
  
bool OntologyNodes::GetCategoryNameStr(CategoryIdType id, std::string& name) const
{
  CategoryNameType uname;
  bool b = GetCategoryName(id, uname);
  if(!b) return false;
  uname.convertString(name, izenelib::util::UString::UTF_8);
  return true;
}






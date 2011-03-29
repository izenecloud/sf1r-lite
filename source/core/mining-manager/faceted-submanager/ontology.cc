#include "ontology.h"
#include "manmade_doc_category_item.h"
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/algorithm/string/split.hpp>
#include <util/filesystem.h>


using namespace sf1r::faceted;

Ontology::Ontology():is_open_(false), category_storage_(new CategoryStorageType()), docs_storage_(new DocsStorageType())
{
}
Ontology::~Ontology()
{
  delete category_storage_;
  delete docs_storage_;
}
    
bool Ontology::Load(const std::string& dir)
{
  try
  {
    if( !boost::filesystem::exists(dir) ) return false;
    std::string name_file = dir+"/name";
    if( !boost::filesystem::exists(name_file) ) return false;
    std::ifstream ifs(name_file.c_str(), std::ios::binary);
    if( ifs.fail()) return false;
    {
      boost::archive::text_iarchive ia(ifs);
      ia >> name_ ;
    }
    ifs.close();
    if( ifs.fail() )
    {
      return false;
    }
    if( !category_storage_->Open(dir+"/nodes") ) return false;
    if( !docs_storage_->Open(dir+"/docs") ) return false;
    if( !LoadCidDocCount_() ) return false;
    is_open_ = true;
  }
  catch(std::exception& ex)
  {
    return false;
  }
  dir_ = dir;
  return true;
}
    
bool Ontology::Init(const std::string& dir)
{
  try
  {
    if( boost::filesystem::exists(dir) ) return false;
    boost::filesystem::create_directories(dir);
    if( !category_storage_->Open(dir+"/nodes") ) return false;
    if( !docs_storage_->Open(dir+"/docs") ) return false;
    if( !LoadCidDocCount_() ) return false;
    is_open_ = true;
  }
  catch(std::exception& ex)
  {
    return false;
  }
  dir_ = dir;
  return true;
}

bool Ontology::InitOrLoad(const std::string& dir)
{
  try
  {
    if( boost::filesystem::exists(dir) ) return Load(dir);
    else return Init(dir);
  }
  catch(std::exception& ex)
  {
    return false;
  }
  return true;
}
    
bool Ontology::Copy(const std::string& from_dir, const std::string& to_dir)
{
  try
  {
    if(boost::filesystem::exists(from_dir) && !boost::filesystem::exists(to_dir))
    {
      izenelib::util::recursive_copy_directory(from_dir, to_dir);
    }
    else
    {
      return false;
    }
  }
  catch(std::exception& ex)
  {
    return false;
  }
  
  return true;
}

bool Ontology::IsOpen() const
{
  return is_open_;
}



bool Ontology::LoadCidDocCount_()
{
  std::stack<CategoryIdDepth> items;
  CategoryIdDepth top(TopCategoryId(), 0);
  ForwardIterate_(top, items);
  std::stack<DocListDepth> doclist_stack;

  while( !items.empty() )
  {
    CategoryIdDepth item = items.top();
    items.pop();
    DepthType depth = item.second;
    CategoryIdType cid = item.first;
    DocidListType docid_list;
    docs_storage_->GetDocSupport(cid, docid_list);
    while( !doclist_stack.empty() )
    {
      DocListDepth doclist_depth = doclist_stack.top();
      if(depth < doclist_depth.second )
      {
        docid_list.insert(docid_list.end(), doclist_depth.first.begin(), doclist_depth.first.end());
        OntologyDocs::SortAndUnique(docid_list);
        doclist_stack.pop();
      }
      else
      {
        break;
      }
    }
    uint32_t count = docid_list.size();
    doclist_stack.push(DocListDepth(docid_list, depth) );
    SetDocCount_(cid, count);
    
  }
  return true;
}

bool Ontology::AddCategoryNode(CategoryIdType parent_id, const CategoryNameType& name, CategoryIdType& id)
{
  return category_storage_->AddCategoryNode(parent_id, name, id);
}

bool Ontology::RenameCategory(CategoryIdType id, const CategoryNameType& new_name)
{
  return category_storage_->RenameCategory(id, new_name);
}

bool Ontology::MoveCategory(CategoryIdType id, CategoryIdType new_parent_id)
{
  return category_storage_->MoveCategory(id, new_parent_id);
}

bool Ontology::DelCategory(CategoryIdType id)
{
  boost::lock_guard<boost::shared_mutex> lock(mutex_);
  //recursive delete
  std::queue<CategoryIdType> items;
  PostIterate_(id, items);
  while( !items.empty())
  {
    CategoryIdType cid = items.front();
    items.pop();
    if( !docs_storage_->DelCategory(cid) ) return false;
    if( !category_storage_->DelCategory(cid) ) return false;
  }
  return true;
}

bool Ontology::InsertDoc(CategoryIdType cid, uint32_t doc_id)
{
  boost::lock_guard<boost::shared_mutex> lock(mutex_);
//   std::cout<<"[C2D] find docid "<<doc_id<<" , cid: "<<cid<<std::endl;
  return docs_storage_->InsertDoc(cid, doc_id);
}

bool Ontology::DelDoc(CategoryIdType cid, uint32_t doc_id)
{
  boost::lock_guard<boost::shared_mutex> lock(mutex_);
  return docs_storage_->DelDoc(cid, doc_id);
}

bool Ontology::ApplyModification()
{
  boost::lock_guard<boost::shared_mutex> lock(mutex_);
  if( !category_storage_->Save() ) return false;
  if( !docs_storage_->ApplyModification() ) return false;
  //rebuild cid_doccount
  if( !LoadCidDocCount_() ) return false;
  return true;
}

std::string Ontology::GetName() const
{
  return name_;
}

bool Ontology::SetName(const std::string& name)
{
  name_ = name;
  std::string name_file = dir_+"/name";
  std::ofstream ofs(name_file.c_str());
  if( ofs.fail()) return false;
  {
    boost::archive::text_oarchive oa(ofs);
    oa << name ;
  }
  ofs.close();
  if( ofs.fail() )
  {
    return false;
  }
  return true;
}

bool Ontology::GetRepresentation(OntologyRep& rep, bool include_top)
{
  CategoryIdDepth top(TopCategoryId(), 0);
  return GetRepresentation_(top, rep, include_top);
}

bool Ontology::GetDocSupport(CategoryIdType id, DocidListType& docid_list)
{
  boost::shared_lock<boost::shared_mutex> mLock(mutex_);
  return docs_storage_->GetDocSupport(id, docid_list);
}

bool Ontology::GetRecursiveDocSupport(CategoryIdType id, DocidListType& docid_list)
{
  bool b = GetRecursiveDocSupport_(id, docid_list);
  if(!b) return false;
  OntologyDocs::SortAndUnique(docid_list);
  return true;
}

bool Ontology::GetDocCount(CategoryIdType id, uint32_t& count) const
{
  if( id>= cid_doccount_.size() ) return false;
  count = cid_doccount_[id];
  return true;
}

bool Ontology::GetCategories(uint32_t docid, CidListType& cid_list)
{
  boost::shared_lock<boost::shared_mutex> mLock(mutex_);
  return docs_storage_->GetCategories(docid, cid_list);
}
    
bool Ontology::GetParent(CategoryIdType id, CategoryIdType& parent_id) const
{
  return category_storage_->GetParent(id, parent_id);
}

bool Ontology::GetCategoryName(CategoryIdType id, CategoryNameType& name) const
{
  return category_storage_->GetCategoryName(id, name);
}

bool Ontology::GetCategoryRule(CategoryIdType id, OntologyNodeRule& rule) const
{
  return category_storage_->GetCategoryRule(id, rule);
}

bool Ontology::SetCategoryRule(CategoryIdType id, const OntologyNodeRule& rule)
{
  return category_storage_->SetCategoryRule(id, rule);
}

bool Ontology::IsLeafCategory(CategoryIdType id)
{
  CategoryStorageType::ChildrenType children;
  if( category_storage_->GetChildren(id, children) )
  {
    return children.empty();
  }
  else
  {
    return false;
  }
}

bool Ontology::GenCategoryId(std::vector<ManmadeDocCategoryItem>& items)
{
  if(items.empty()) return true;
  izenelib::am::rde_hash<CategoryNameType, CategoryIdType> map;
  for(CategoryIdType cid=1;cid<GetCidCount();cid++)
  {
    CategoryNameType name;
    if(GetCategoryName(cid, name))
    {
      map.insert(name, cid);
    }
  }
  for(uint32_t i=0;i<items.size();i++)
  {
    if(!map.get(items[i].cname, items[i].cid))
    {
      items[i].cid = 0;
    }
  }
  return true;
}

bool Ontology::DocInCategory(uint32_t docid, CategoryIdType cid)
{
  CidListType cid_list;
  if( GetCategories(docid, cid_list) )
  {
    CidListType::iterator cit = cid_list.begin();
    while( cit!=cid_list.end() )
    {
      CategoryIdType this_cid = *cit;
      while(true)
      {
        if(this_cid == cid) return true;
        else if(this_cid == TopCategoryId() ) break;
        else
        {
          CategoryIdType parent_id;
          if( !GetParent(this_cid, parent_id) )
          {
            std::cerr<<"can not parent id for "<<this_cid<<std::endl;
            break;
          }
          this_cid = parent_id;
        }
      }
      ++cit;
    }
  }
  else
  {
    return false;
  }
  return false;
}

void Ontology::SetDocCount_(CategoryIdType id, uint32_t count)
{
  if( id>= cid_doccount_.size() )
  {
    cid_doccount_.resize(id+1, 0);
  }
  cid_doccount_[id] = count;
}

void Ontology::ForwardIterate_(const CategoryIdDepth& id_depth, std::stack<CategoryIdDepth>& items)
{
  items.push(id_depth);
  CategoryStorageType::ChildrenType children;
  if( category_storage_->GetChildren(id_depth.first, children) )
  {
    CategoryStorageType::ChildrenType::iterator it = children.begin();
    while( it!= children.end() )
    {
      CategoryIdDepth c(*it, id_depth.second+1);
      ForwardIterate_(c, items);
      ++it;
    }
  }
}

void Ontology::PostIterate_(const CategoryIdType& id, std::queue<CategoryIdType>& items)
{
  CategoryStorageType::ChildrenType children;
  if( category_storage_->GetChildren(id, children) )
  {
    CategoryStorageType::ChildrenType::iterator it = children.begin();
    while( it!= children.end() )
    {
      PostIterate_(*it, items);
      ++it;
    }
  }
  items.push(id);
}

bool Ontology::GetRepresentation_(const CategoryIdDepth& id_depth, OntologyRep& rep, bool include_top)
{
//   std::cout<<"G: "<<id_depth.first<<","<<id_depth.second<<std::endl;
  if(include_top || id_depth.first!=TopCategoryId())
  {
    OntologyRepItem rep_item;
    rep_item.level = id_depth.second;
    rep_item.id = id_depth.first;
    if( !category_storage_->GetCategoryName(id_depth.first, rep_item.text) )
    {
      std::cerr<<"can not get cname for "<<id_depth.first<<std::endl;
      return false;
    }
    if( !GetDocCount( id_depth.first, rep_item.doc_count) ) 
    {
      std::cerr<<"can not get doccount for "<<id_depth.first<<std::endl;
      return false;
    }
    rep.item_list.push_back(rep_item);
  }
  CategoryStorageType::ChildrenType children;
  if( category_storage_->GetChildren(id_depth.first, children) )
  {
    CategoryStorageType::ChildrenType::iterator it = children.begin();
    while( it!= children.end() )
    {
      CategoryIdDepth c(*it, id_depth.second+1);
      if( !GetRepresentation_(c, rep, include_top) ) return false;
      ++it;
    }
  }
  return true;
}

bool Ontology::GetRecursiveDocSupport_(CategoryIdType id, DocidListType& docid_list)
{
  DocidListType this_list;
  GetDocSupport(id, this_list);
  if(!this_list.empty())
  {
    docid_list.insert(docid_list.end(), this_list.begin(), this_list.end() );
  }
  CategoryStorageType::ChildrenType children;
  if( category_storage_->GetChildren(id, children) )
  {
    CategoryStorageType::ChildrenType::iterator it = children.begin();
    while( it!= children.end() )
    {
      if( !GetRecursiveDocSupport_(*it, docid_list) ) return false;
      ++it;
    }
  }
  return true;
}

bool Ontology::AddCategoryNodeStr(CategoryIdType parent_id, const std::string& name, CategoryIdType& id)
{
  CategoryNameType uname(name, izenelib::util::UString::UTF_8);
  return AddCategoryNode(parent_id, uname, id);
}

bool Ontology::GetXmlNode_(izenelib::util::ticpp::Element* node, XmlNode& xml_node)
{
  std::string refid = node->GetAttribute("rdf:ID");
  if(refid.empty()) return false;
  xml_node.name = izenelib::util::UString(refid, izenelib::util::UString::UTF_8);
  izenelib::util::ticpp::Element* sub_node = node->FirstChildElement( "rdfs:subClassOf", false );
  if(sub_node!= NULL)
  {
    std::string parent_name = sub_node->GetAttribute("rdf:resource");
    if(parent_name.empty()||parent_name[0]!='#')
    {
      std::cerr<<"Parent name failed: "<<parent_name<<std::endl;
      return false;
    }
    parent_name = parent_name.substr(1, parent_name.length()-1);
    xml_node.parent_name = izenelib::util::UString(parent_name, izenelib::util::UString::UTF_8);
    izenelib::util::ticpp::Element* label_node = node->FirstChildElement( "rdfs:label", false );
    izenelib::util::ticpp::Element* des_node = node->FirstChildElement( "rdfs:comment", false );
    if(label_node!=NULL)
    {
      std::string label_str = label_node->GetText(false);
      std::vector<std::string> str_labels;
      boost::algorithm::split( str_labels, label_str, boost::algorithm::is_any_of(",") );
      xml_node.rule.labels.resize(str_labels.size());
      for(uint32_t i=0;i<str_labels.size();i++)
      {
        xml_node.rule.labels[i] = izenelib::util::UString(str_labels[i], izenelib::util::UString::UTF_8);
      }
    }
    if(des_node!=NULL)
    {
      std::string des_str = des_node->GetText(false);
      xml_node.rule.description = izenelib::util::UString(des_str, izenelib::util::UString::UTF_8);
    }
  }
  return true;
}

bool Ontology::SetXML(const std::string& xml)
{
  using namespace izenelib::util::ticpp;
  
  std::vector<CategoryIdType> delete_list;
  izenelib::util::ticpp::Document xml_doc;
  try
  {
    xml_doc.Parse(xml);
  }
  catch(std::exception& ex)
  {
    std::cerr<<ex.what()<<std::endl;
    return false;
  }
  izenelib::util::ticpp::Element* top = xml_doc.FirstChildElement( "rdf:RDF", false );
  if(top==NULL)
  {
    std::cerr<<"Can not find rdf:RDF"<<std::endl;
    return false;
  }
  izenelib::util::ticpp::Element* onto = top->FirstChildElement( "owl:Ontology", false );
  if(onto==NULL)
  {
    std::cerr<<"Can not find owl:Ontology"<<std::endl;
    return false;
  }
  izenelib::util::ticpp::Element* name_node = onto->FirstChildElement( "rdfs:comment", false );
  if(name_node==NULL)
  {
    std::cerr<<"Can not find name_node"<<std::endl;
    return false;
  }
  std::string name = name_node->GetText(false);
  boost::algorithm::trim(name);
  Iterator<Element> it( "owl:Class" );
  std::vector<XmlNode> xml_nodes;
  for ( it = it.begin( top ); it != it.end(); it++ )
  {
    XmlNode xml_node;
    if(!GetXmlNode_(it.Get(), xml_node))
    {
      std::cerr<<"Get node in xml failed."<<std::endl;
      return false;
    }
    xml_nodes.push_back(xml_node);
//     std::cout<<"P:"<<xml_node.parent_name<<", T:"<<xml_node.name<<std::endl;
//     if( !docs_storage_->DelCategory(cid)) return false;
  }
  
  if(xml_nodes.empty())
  {
    std::cerr<<"Empty category."<<std::endl;
    return false;
  }
  CategoryNameType parent_name = CategoryStorageType::GetTopName();
  izenelib::am::rde_hash<CategoryNameType, CategoryIdType> parents;
  parents.insert(parent_name, TopCategoryId());
  CategoryIdType* pid = NULL;
  while(true)
  {
    std::vector<std::pair<CategoryNameType,CategoryIdType> > candidate_parent;
    for(uint32_t i=0;i<xml_nodes.size();i++)
    {
      
      if( (pid=parents.find(xml_nodes[i].parent_name))!=NULL)
      {
        CategoryIdType id;
        if(!AddCategoryNode(*pid, xml_nodes[i].name, id))
        {
          std::cerr<<"Add category node failed."<<std::endl;
          return false;
        }
//         std::cout<<"P:"<<xml_nodes[i].parent_name<<", "<<*pid<<", T:"<<xml_nodes[i].name<<", I:"<<id<<std::endl;
        candidate_parent.push_back(std::make_pair(xml_nodes[i].name, id)); 
        //set rule if any
        SetCategoryRule(id, xml_nodes[i].rule);
      }
    }
    if(candidate_parent.empty()) break;
    else
    {
      parents.clear();
      for(uint32_t i=0;i<candidate_parent.size();i++)
      {
        parents.insert(candidate_parent[i].first, candidate_parent[i].second);
      }
    }
  }
  
  if(!SetName(name))
  {
    std::cerr<<"Can not set name to "<<name<<std::endl;
    return false;
  }
  if(!ApplyModification())
  {
    std::cerr<<"Can not ApplyModification"<<std::endl;
    return false;
  }
  return true;
}

bool Ontology::CopyDocsFrom(Ontology* ontology)
{
  izenelib::am::rde_hash<CategoryNameType, CategoryIdType> this_map;
  std::vector<CategoryNameType> this_list;
  for(CategoryIdType cid=1;cid<GetCidCount();cid++)
  {
    CategoryNameType name;
    if(GetCategoryName(cid, name))
    {
      this_map.insert(name, cid);
      this_list.push_back(name);
    }
  }
  
  izenelib::am::rde_hash<CategoryNameType, CategoryIdType> another_map;
  for(CategoryIdType cid=1;cid<ontology->GetCidCount();cid++)
  {
    CategoryNameType name;
    if(ontology->GetCategoryName(cid, name))
    {
      another_map.insert(name, cid);
    }
  }
  for(uint32_t i=0;i<this_list.size();i++)
  {
    CategoryIdType* another_cid = another_map.find(this_list[i]);
    CategoryIdType* cid = this_map.find(this_list[i]);
    if(another_cid!=NULL&&cid!=NULL)
    {
      DocidListType docid_list;
      if(ontology->Docs()->GetDocSupport(*another_cid, docid_list))
      {
        Docs()->SetDocSupport(*cid, docid_list);
      }
    }
  }
  return true;
}
    
bool Ontology::GetXML(std::string& xml)
{
  OntologyRep rep;
  if(!GetRepresentation(rep, true)) return false;
  std::stringstream ss;
  ss<<"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"<<std::endl;
  ss<<"<rdf:RDF xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\""<<std::endl;
  ss<<"xmlns:rdfs=\"http://www.w3.org/2000/01/rdf-schema#\""<<std::endl;
  ss<<"xmlns:owl=\"http://www.w3.org/2002/07/owl#\""<<std::endl;
  ss<<"xmlns=\"http://www.xfront.com/owl/ontologies/camera/#\""<<std::endl;
  ss<<"xmlns:camera=\"http://www.xfront.com/owl/ontologies/camera/#\""<<std::endl;
  ss<<"xml:base=\"http://www.xfront.com/owl/ontologies/camera/\">"<<std::endl;
  ss<<"<owl:Ontology rdf:about=\"\">"<<std::endl;
  ss<<"<rdfs:comment>"<<GetName()<<"</rdfs:comment>"<<std::endl;
  ss<<"</owl:Ontology>"<<std::endl;
  ss<<"<owl:AnnotationProperty rdf:about=\"&faceted;id\"/>"<<std::endl;
  ss<<"<owl:AnnotationProperty rdf:about=\"&faceted;doccount\"/>"<<std::endl;
  std::string top_name = "__top";
  std::list<OntologyRepItem>::iterator it = rep.item_list.begin();
  std::stack<OntologyRepItem> path;
  while(it!=rep.item_list.end())
  {
    OntologyRepItem item = (*it);
    ++it;
    std::string name;
    item.text.convertString(name, izenelib::util::UString::UTF_8);
    std::string parent_name;
    while(!path.empty())
    {
      if(path.top().level>=item.level)
      {
        path.pop();
      }
      else
      {
        break;
      }
    }
    if(!path.empty())
    {
      path.top().text.convertString(parent_name, izenelib::util::UString::UTF_8);
    }
    path.push(item);
    ss<<"<owl:Class rdf:ID=\""<<name<<"\">"<<std::endl;
    if(!parent_name.empty())
    {
      ss<<"<rdfs:subClassOf rdf:resource=\"#"<<parent_name<<"\"/>"<<std::endl;
    }
    ss<<"<faceted:id>"<<item.id<<"</faceted:id>"<<std::endl;
    ss<<"<faceted:doccount>"<<item.doc_count<<"</faceted:doccount>"<<std::endl;
    ss<<"<rdfs:label>";
    OntologyNodeRule rule;
    if(category_storage_->GetCategoryRule(item.id, rule))
    {
      for(uint32_t i=0;i<rule.labels.size();i++)
      {
        if(i>0) ss<<",";
        std::string str_label;
        rule.labels[i].convertString(str_label, izenelib::util::UString::UTF_8);
        ss<<str_label;
      }
    }
    ss<<"</rdfs:label>"<<std::endl;
    std::string str_description;
    rule.description.convertString(str_description, izenelib::util::UString::UTF_8);
    ss<<"<rdfs:comment>"<<str_description<<"</rdfs:comment>"<<std::endl;
    ss<<"</owl:Class>"<<std::endl;
  }
  ss<<"</rdf:RDF>"<<std::endl;
  xml = ss.str();
  return true;
}


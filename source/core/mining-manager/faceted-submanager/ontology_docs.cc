#include "ontology_docs.h"
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/vector.hpp>
using namespace sf1r::faceted;

OntologyDocs::OntologyDocs():is_open_(false),cid2doc_(NULL)
{
}

OntologyDocs::~OntologyDocs()
{
  if(IsOpen())
  {
    cid2doc_->close();
    delete cid2doc_;
  }
}

bool OntologyDocs::IsOpen() const
{
  return is_open_;
}

bool OntologyDocs::Open(const std::string& dir)
{
  dir_ = dir;
  try
  {
    if(!boost::filesystem::exists(dir))
    {
      boost::filesystem::create_directories(dir);
    }
  }
  catch(std::exception& ex)
  {
    return false;
  }
  std::string cid2doc_file = dir+"/cid2doc";
  std::string doc2cid_file = dir+"/doc2cid";
  cid2doc_ = new Cid2DocType(cid2doc_file);
  cid2doc_->open();
  if(boost::filesystem::exists(doc2cid_file))
  {
    std::ifstream ifs(doc2cid_file.c_str(), std::ios::binary);
    if( ifs.fail()) return false;
    {
      boost::archive::text_iarchive ia(ifs);
      ia >> doc2cid_ ;
    }
    ifs.close();
    if( ifs.fail() )
    {
      return false;
    }
  }
  is_open_ = true;
  return true;
}
  
// bool OntologyDocs::CopyFrom(const std::string& from_dir, const std::string& to_dir)
// {
//   try
//   {
//     if(boost::filesystem::exists(from_dir) && !boost::filesystem::exists(to_dir))
//     {
//       boost::filesystem::copy(from_dir, to_dir);
//     }
//     else
//     {
//       return false;
//     }
//   }
//   catch(std::exception& ex)
//   {
//     return false;
//   }
//   return true;
// }
  
bool OntologyDocs::InsertDoc(CategoryIdType cid, uint32_t doc_id)
{
  
  if( doc_id ==0) return false;
  OntologyDocsOp op;
  op.op = OntologyDocsOp::INS;
  op.cid = cid;
  op.docid = doc_id;
  changes_.push_back(op);
  return true;
}
  
bool OntologyDocs::DelDoc(CategoryIdType cid, uint32_t doc_id)
{
  
  if( doc_id ==0) return false;
  OntologyDocsOp op;
  op.op = OntologyDocsOp::DEL;
  op.cid = cid;
  op.docid = doc_id;
  changes_.push_back(op);
  return true;
}
  
bool OntologyDocs::DelCategory(CategoryIdType id)
{
  
  DocidListType docid_list;
  if( !cid2doc_->get(id, docid_list) ) return false;
  DocidListType::iterator it = docid_list.begin();
  while( it!= docid_list.end() )
  {
    OntologyDocsOp op;
    op.op = OntologyDocsOp::DEL;
    op.cid = id;
    op.docid = *it;
    changes_.push_back(op);
    ++it;
  }
  return true;
}
  
bool OntologyDocs::ApplyModification()
{
  if( changes_.empty() ) return true;
  std::stable_sort(changes_.begin(), changes_.end(), OntologyDocsOp::op_docid_greater ); // order by docid desc
  
  std::vector<OntologyDocsOp> run_list;
  for(uint32_t i=0;i<changes_.size();i++)
  {
    if(i>0)
    {
      if( run_list.back().docid != changes_[i].docid )
      {
        //process
        ProcessOnDoc_(run_list);
        run_list.resize(0);
      }
    }
    run_list.push_back(changes_[i]);
  }
  //process
  ProcessOnDoc_(run_list);
  run_list.resize(0);
  
  std::stable_sort(changes_.begin(), changes_.end(), OntologyDocsOp::op_cid_less );
  for(uint32_t i=0;i<changes_.size();i++)
  {
    if(i>0)
    {
      if( run_list.back().cid != changes_[i].cid )
      {
        //process
        ProcessOnCategory_(run_list);
        run_list.resize(0);
      }
    }
    run_list.push_back(changes_[i]);
  }
  //process
  ProcessOnCategory_(run_list);
  run_list.resize(0);
  {
    std::string doc2cid_file = dir_+"/doc2cid";
    std::ofstream ofs(doc2cid_file.c_str());
    if( ofs.fail()) return false;
    {
      boost::archive::text_oarchive oa(ofs);
      oa << doc2cid_ ;
    }
    ofs.close();
    if( ofs.fail() )
    {
      return false;
    }
  }
  return true;
}

bool OntologyDocs::SetDocSupport(CategoryIdType id, const DocidListType& docid_list)
{
  try
  {
    cid2doc_->update(id, docid_list);
  }
  catch(std::exception& ex)
  {
    std::cerr<<ex.what()<<std::endl;
    return false;
  }
  return true;
}

bool OntologyDocs::GetDocSupport(CategoryIdType id, DocidListType& docid_list)
{
  bool b = false;
//   std::cout<<"GetDocSupport "<<id<<std::endl;
  try
  {
    b = cid2doc_->get(id, docid_list);
  }
  catch(std::exception& ex)
  {
    std::cerr<<ex.what()<<std::endl;
    b = false;
  }
  return b;
}
  
bool OntologyDocs::GetCategories(uint32_t docid, CidListType& cid_list)
{
  
  if(docid==0||docid>doc2cid_.size() ) return false;
  cid_list = doc2cid_[docid-1];
  return true;
}

void OntologyDocs::ProcessOnDoc_(const std::vector<OntologyDocsOp>& changes_on_doc)
{
  if( changes_on_doc.empty() ) return;
  uint32_t docid = changes_on_doc[0].docid;
  if( docid>doc2cid_.size() ) doc2cid_.resize(docid);
  CidListType& cid_list = doc2cid_[docid-1];
  for(uint32_t i=0;i<changes_on_doc.size();i++)
  {
    if( changes_on_doc[i].op == OntologyDocsOp::INS )
    {
      cid_list.push_back(changes_on_doc[i].cid);
    }
    else
    {
      cid_list.remove(changes_on_doc[i].cid);
    }
  }
  SortAndUnique(cid_list);
}

void OntologyDocs::ProcessOnCategory_(const std::vector<OntologyDocsOp>& changes_on_category)
{
  if( changes_on_category.empty() ) return;
  uint32_t cid = changes_on_category[0].cid;
  DocidListType docid_list;
  cid2doc_->get(cid, docid_list);
  for(uint32_t i=0;i<changes_on_category.size();i++)
  {
    if( changes_on_category[i].op == OntologyDocsOp::INS )
    {
      docid_list.push_back(changes_on_category[i].docid);
    }
    else
    {
      docid_list.remove(changes_on_category[i].docid);
    }
  }
  SortAndUnique(docid_list);
  cid2doc_->update(cid, docid_list);
}



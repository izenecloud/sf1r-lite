#include <mining-manager/util/MUtil.hpp>
#include <mining-manager/util/TermUtil.hpp>
#include <mining-manager/util/FSUtil.hpp>
#include "TaxonomyInfo.h"
#include "TgTypes.h"

#include <la/stem/Stemmer.h>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <util/profiler/ProfilerGroup.h>
#include <am/3rdparty/rde_hash.h>
#include <ctime>
#include "math.h"
using namespace sf1r;
namespace bfs=boost::filesystem;


idmlib::NameEntityManager* TaxonomyInfo::nec_ = NULL;

TaxonomyInfo::TaxonomyInfo (
  const std::string& path,
  const TaxonomyPara& taxonomy_param,
  boost::shared_ptr<LabelManager> labelManager,
  idmlib::util::IDMAnalyzer* analyzer,
  const std::string& sys_res_path) :
  path_(path), taxonomy_param_(taxonomy_param), analyzer_(analyzer),id_manager_(NULL)
  , labelManager_(labelManager), sys_res_path_(sys_res_path)
  , kpe_(NULL), scorer_(NULL)
  , enable_new_nec_(true), cn_nec_(NULL)
  , kpeCacheSize_(0)
  , max_docid_(0), last_insert_docid_(0), insert_count_(0), label_processed_(0)
  , max_docid_file_(path+"/max_id")
{
  
}

bool TaxonomyInfo::Open()
{
  if( max_docid_file_.Load() )
  {
    max_docid_ = max_docid_file_.GetValue();
  }
  
  boost::filesystem::create_directories(path_+"/id/");
  id_manager_ = new idmlib::util::IDMIdManager(path_+"/id/");
  
  callback_func_ = boost::bind( &TaxonomyInfo::callback_, this, _1, _2, _3, _4, _5);
  
  if( !boost::filesystem::exists(sys_res_path_) ) return false;
  std::string kpe_res_path = sys_res_path_+"/kpe";
  if( !boost::filesystem::exists(kpe_res_path) ) return false;
  scorer_ = new kpe_type::ScorerType(analyzer_);
  if(!scorer_->load(kpe_res_path))
  {
    return false;
  }
  
  if( taxonomy_param_.enable_nec )
  {
      std::string necPath = sys_res_path_+"/nec";
      std::cout<<"NEC path : "<<necPath<<std::endl;
      if( !boost::filesystem::exists(necPath) ) return false;
      idmlib::NameEntityManager& nec = idmlib::NameEntityManager::getInstance(necPath);
      nec_ = &nec;
      if(enable_new_nec_)
      {
        cn_nec_ = new idmlib::nec::NEC();
        if(!cn_nec_->Load(necPath+"/chinese_svm"))
        {
          std::cout<<"load chinese_svm failed."<<std::endl;
          return false;
        }
      }
  }
  else
  {
      std::cout<<"NEC disabled."<<std::endl;
  }
  
  std::string kp_continue_file = path_+"/kp_continue";
  if(boost::filesystem::exists(kp_continue_file))
  {
    std::ifstream ifs(kp_continue_file.c_str());
    std::string line;
    getline(ifs, line);
    max_docid_ = boost::lexical_cast<uint32_t>(line);
    ifs.close();
    FSUtil::del(kp_continue_file);
  }
  else
  {
    FSUtil::del(path_+"/fragments");
  }
  kpe_ = initKPE_();
  
  
  return true;
}

TaxonomyInfo::~TaxonomyInfo()
{
  if(scorer_!=NULL)
    delete scorer_;
  if(id_manager_!=NULL)
    delete id_manager_;
  if(kpe_!=NULL)
    delete kpe_;
  if(cn_nec_!=NULL)
  {
    delete cn_nec_;
  }
}

TaxonomyInfo::kpe_type* TaxonomyInfo::initKPE_()
{
  std::string dir = path_+"/fragments";
  boost::filesystem::create_directories(dir);
  kpe_type* kpe = new kpe_type(dir, analyzer_ ,callback_func_, id_manager_);
  kpe->set_cache_size(kpeCacheSize_);
  kpe->set_scorer( scorer_ );
  return kpe;
}

void TaxonomyInfo::addDocument(uint32_t docid, const izenelib::util::UString& text)
{
//   std::cout<<"[TI] add "<<docid<<std::endl;
  kpe_->insert(text, docid);
  if( docid > max_docid_ ) max_docid_ = docid;
  if( last_insert_docid_ != docid) ++insert_count_;
  last_insert_docid_ = docid;
}


void TaxonomyInfo::callback_(
const izenelib::util::UString& str
, const std::vector<id2count_t>& id2countList
, uint8_t score
, const std::vector<id2count_t>& leftTermList
, const std::vector<id2count_t>& rightTermList)
{
  label_processed_++;
  uint8_t labelType = LabelManager::LABEL_TYPE::COMMON;
  
  if( nec_ != NULL )
  {
    if(!labelManager_->ExistsLabel(str) )
    {
      bool has_cn_char = false;
      bool all_cn_char = true;
      for(uint32_t i=0;i<str.length();i++)
      {
        if(!str.isChineseChar(i))
        {
          all_cn_char = false;
          break;
        }
      }
      for(uint32_t i=0;i<str.length();i++)
      {
        if(str.isChineseChar(i))
        {
          has_cn_char = true;
          break;
        }
      }
      
      if(!has_cn_char || all_cn_char)
      {
        if(enable_new_nec_ && all_cn_char )
        {
          idmlib::nec::NECItem nec_item;
          nec_item.surface = str;
          nec_item.id2countList = id2countList;
          std::vector<str2count_t> leftTermStrList(leftTermList.size());
          std::vector<str2count_t> rightTermStrList(rightTermList.size());
          for(uint32_t i=0;i<leftTermList.size();i++)
          {
              izenelib::util::UString prefix;
              if( id_manager_->GetStringById(leftTermList[i].first, prefix) )
              {
                  leftTermStrList[i].first = prefix;
                  leftTermStrList[i].second = leftTermList[i].second;
              }
          }
          
          for(uint32_t i=0;i<rightTermList.size();i++)
          {
              izenelib::util::UString suffix;
              if( id_manager_->GetStringById(rightTermList[i].first, suffix) )
              {
                  rightTermStrList[i].first = suffix;
                  rightTermStrList[i].second = rightTermList[i].second;
              }
          }
          nec_item.leftTermList = leftTermStrList;
          nec_item.rightTermList = rightTermStrList;
          labelType = cn_nec_->Predict(nec_item);
        }
        else
        {
          std::vector<izenelib::util::UString> prefixList;
          for(uint32_t i=0;i<leftTermList.size();i++)
          {
              izenelib::util::UString prefix;
              if( id_manager_->GetStringById(leftTermList[i].first, prefix) )
              {
                  prefixList.push_back(prefix);
              }
          }
          
          std::vector<izenelib::util::UString> suffixList;
          for(uint32_t i=0;i<rightTermList.size();i++)
          {
              izenelib::util::UString suffix;
              if( id_manager_->GetStringById(rightTermList[i].first, suffix) )
              {
                  suffixList.push_back(suffix);
              }
          }
          
          idmlib::NameEntity ne(str, prefixList, suffixList);
          
          nec_->predict(ne);
          std::vector<ml::Label>& mllabels= ne.predictLabels;
          
          for(uint32_t i=0;i<mllabels.size();i++)
          {
              if(mllabels[i] == "PEOP" )
              {
                  labelType = LabelManager::LABEL_TYPE::PEOP;;
                  break;
              }
              else if( mllabels[i] == "LOC" )
              {
                  labelType = LabelManager::LABEL_TYPE::LOC;;
                  break;
              }
              else if( mllabels[i] == "ORG" )
              {
                  labelType = LabelManager::LABEL_TYPE::ORG;;
                  break;
              }
          }
        }
      }
    }
//         std::string ss;
//         str.convertString(ss, izenelib::util::UString::UTF_8);
//         std::cout<<"label type - "<<ss<<" : "<<(uint32_t)labelType<<std::endl;
  }
  labelManager_->insertLabel(str, id2countList, score, labelType);
  if(label_processed_%1000==0)
  {
    MEMLOG("[TgInfo] processed %d labels.", label_processed_);
  }
    //below for debug output
    
}

bool TaxonomyInfo::deleteDocument(uint32_t docid)
{
    labelManager_->deleteDocLabels(docid);
    return true;
}

void TaxonomyInfo::finish()
{
  if( insert_count_>=0 )//true
  {
    kpe_->close();
    labelManager_->buildDocLabelMap();
  }
  max_docid_file_.SetValue(max_docid_);
  max_docid_file_.Save();
  label_processed_ = 0;
  last_insert_docid_ = 0;
  insert_count_ = 0;
}

void TaxonomyInfo::ReleaseResource()
{
  if(scorer_!=NULL)
  {
    delete scorer_;
    scorer_ = NULL;
  }
}

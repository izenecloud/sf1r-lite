#include "DupDetector2.h"
#include "dd_constants.h"
#include <util/CBitArray.h>
#include <util/ClockTimer.h>
#include <utility>

#include <boost/lexical_cast.hpp>

// #define DUPD_GROUP_DEBUG;
// #define DUPD_TEXT_DEBUG;
// #define DUPD_DEBUG;
// #define DUPD_FP_DEBUG;

using namespace std;
namespace sf1r{

DupDetector2::DupDetector2(const std::string& container)
:container_(container), document_manager_(), analyzer_(NULL) 
, file_info_(new FileObjectType(container+"/file_obj"))
, maxk_(0), partition_num_(0)
, algo_(NULL)
, fp_storage_(container+"/fp")
, group_(NULL)
, process_start_docid_(0), process_max_docid_(0)
, max_docid_(0)
{
  SetParameters_();
}

DupDetector2::DupDetector2(const std::string& container, const boost::shared_ptr<DocumentManager>& document_manager, const std::vector<std::string>& properties, idmlib::util::IDMAnalyzer* analyzer)
:container_(container), document_manager_(document_manager), analyzer_(analyzer) 
, file_info_(new FileObjectType(container+"/file_obj"))
, maxk_(0), partition_num_(0)
, algo_(NULL)
, fp_storage_(container+"/fp")
, group_(NULL)
, process_start_docid_(0), process_max_docid_(0)
, max_docid_(0)
{
  SetParameters_();
  for(uint32_t i=0;i<properties.size();i++)
  {
    dd_properties_.insert( properties[i], 0);
  }
}

void DupDetector2::SetParameters_()
{
  maxk_ = 6;
  partition_num_ = 9;
  trash_threshold_ = 0.02;
  trash_min_ = 200;
}

uint8_t DupDetector2::GetK_(uint32_t doc_length)
{
  // (document length, k) follows a log based regression function
  // while the document length gets larger, k value decreases.
  // 1 is added to doc_length because ln(0) is not defined (1 is a kind of smothing factor)
//   return 3;
  double y = -1.619429845 * log((double)(doc_length + 1)) + 17.57504447;
  if(y <= 0) y = 0;
  uint8_t k = (uint8_t)y;
  if(y-(double)k>=0.5) k++;
  if(k > 6) k = 6;

  return k;
}

DupDetector2::~DupDetector2() {
  delete file_info_;
  if(algo_)
    delete algo_;
  if(group_)
    delete group_;
}

bool DupDetector2::Open()
{
  std::string fp_dir = container_+"/fp";
  try
  {
    boost::filesystem::create_directories(container_);
    boost::filesystem::create_directories(fp_dir);
  }
  catch(std::exception& ex)
  {
    std::cerr<<ex.what()<<std::endl;
    return false;
  }
  
  if( file_info_->Load() )
  {
    max_docid_ = file_info_->GetValue();
//     maxk_ = file_info_.GetValue().second;
  }
  algo_ = new CharikarAlgo(container_.c_str(), DdConstants::f, 0.95);
  FpTables tables;
  tables.GenTables(DdConstants::f, maxk_, partition_num_, table_list_);
  std::cout<<"Generated "<<table_list_.size()<<" tables for maxk="<<(int)maxk_<<std::endl;
  group_ = new GroupType( (container_+"/group").c_str() );
  if(!group_->Load())
  {
    std::cerr<<"Load group info failed"<<std::endl;
  }
  std::string continue_file = container_+"/continue";
  try
  {
    if(boost::filesystem::exists(continue_file))
    {
      boost::filesystem::remove_all(continue_file);
      runDuplicateDetectionAnalysis(true);
    }
  }
  catch(std::exception& ex)
  {
    std::cerr<<ex.what()<<std::endl;
  }
  return true;
//   uint8_t sort_count = FPCompareType::GetSortCount();
//   fp_groups_.resize(sort_count);
//   compare_obj_.resize(sort_count);
//   for( uint32_t i=0;i<sort_count;i++)
//   {
//     compare_obj_[i] = FPCompareType(i);
//     fp_groups_[i] = nm_+"/fp_group"+boost::lexical_cast<std::string>(i+1);
//     
//   }
}

void DupDetector2::Resize(uint32_t size)
{
  vec_.resize(size);
}

bool DupDetector2::SkipTrash_(uint32_t total_count, uint32_t pairwise_count, uint32_t table_id)
{
  if(table_id==0) return false; //first table.
  if(pairwise_count<trash_min_) return false;
  else return true;
//   double d = (double)pairwise_count/total_count;
//   if(d>=trash_threshold_)
//   {
//     return true;
//   }
//   return false;
}

void DupDetector2::FindDD_(const std::vector<FpItem>& data, const FpTable& table, uint32_t table_id)
{
  std::vector<FpItem > for_compare;
  bool is_first = true;
  uint64_t last_compare_value = 0;
  uint32_t this_max_docid = 0;
  uint32_t total_count = data.size();
  uint32_t dd_count = 0;
  for(uint32_t i=0;i<data.size();i++)
  {
    uint32_t docid = data[i].docid;
    if(docid==0) continue;
//     if(i%10000==0) std::cout<<"[dup-id] "<<i<<std::endl;
    uint64_t compare_value = table.GetBitsValue(data[i].fp);
    if( is_first )
    {
    }
    else
    {
      if( last_compare_value != compare_value )
      {
        //process for_compare
        if( this_max_docid >= process_start_docid_) 
        {
          if(!SkipTrash_(total_count, for_compare.size(), table_id))
            PairWiseCompare_(for_compare, dd_count);
        }
        for_compare.resize(0);
        this_max_docid = 0;
      }
    }
    for_compare.push_back(data[i]);
    last_compare_value = compare_value;
    
    if( docid>this_max_docid )
    {
      this_max_docid=  docid;
    }
    is_first = false;
  }
  if( this_max_docid >= process_start_docid_) 
  {
    if(!SkipTrash_(total_count, for_compare.size(), table_id))
      PairWiseCompare_(for_compare, dd_count);
  }
  for_compare.resize(0);
  std::cout<<"table id "<<table_id<<" find "<<dd_count<<" dd pairs."<<std::endl;
}

void DupDetector2::PairWiseCompare_(const std::vector<FpItem>& for_compare, uint32_t& dd_count)
{
  if(for_compare.size()==0) return;
#ifdef DUPD_GROUP_DEBUG
  std::cout<<"[in-group] "<<for_compare.size()<<std::endl;
#endif
  for( uint32_t i=0;i<for_compare.size();i++)
  {
    for(uint32_t j=i+1;j<for_compare.size();j++)
    {
      if( for_compare[i].docid<process_start_docid_ && for_compare[j].docid<process_start_docid_ ) continue;
      if( IsDuplicated_(for_compare[i], for_compare[j] ) )
      {
        ++dd_count;
        boost::lock_guard<boost::shared_mutex> lock(read_write_mutex_);
        group_->AddDoc( for_compare[i].docid, for_compare[j].docid );

      }

    }
  }
}

bool DupDetector2::IsDuplicated_(const FpItem& item1, const FpItem& item2)
{
  izenelib::util::CBitArray c = item1.fp ^ item2.fp;
  bool result = false;

  // K should be selected by longer document. (longer document generates lower K value)
  // Therefore, std::max is used to get longer document.
  uint32_t doc_length = std::max(item1.length, item2.length);

  // We need to check the corpus is based on English alphabet.
  // Because GetK_ function is based on actual document length.
  // In here, 7 is multiplied to document length. (7 means the average word length in Enlgish)
//   bool isEnglish = true;
//   if(isEnglish)
    
  doc_length *= 7;
  uint8_t k = GetK_(doc_length);
  if( c.GetCount() <= k )
  {
    result = true;
  }
#ifdef DUPD_DEBUG
  if(result)
  {
  std::cout<<"find dd: "<<item1.docid<<" , "<<item2.docid<<" : "<<(uint32_t)k<<","<<c.GetCount()<<std::endl;
  }
#endif
//   std::cout<<"D"<<item1.docid<<",D"<<item2.docid<<" "<<c.GetCount()<<" "<<item1.length<<","<<item2.length<<" "<<(int)result<<std::endl;
  return result;
}

// void DupDetector2::DataDupd(const std::vector<std::pair<uint32_t, izenelib::util::CBitArray> >& data, uint8_t group_num, uint32_t min_docid)
// {
//   std::vector<std::pair<uint32_t, izenelib::util::CBitArray> > for_compare;
//   bool is_first = true;
//   uint64_t last_compare_value = 0;
//   uint32_t this_max_docid = 0;
//   for(uint32_t i=0;i<data.size();i++)
//   {
//     uint32_t docid = data[i].first;
//     uint64_t compare_value = compare_obj_[group_num].GetBitsValue(data[i].second);
//     if( is_first )
//     {
//     }
//     else
//     {
//       
//       if( last_compare_value != compare_value )
//       {
//         //process for_compare
//         if( this_max_docid >= min_docid) PairWiseCompare_(for_compare, min_docid);
//         for_compare.resize(0);
//         this_max_docid = 0;
//       }
//     }
//     for_compare.push_back(data[i]);
//     last_compare_value = compare_value;
//     
//     if( docid>this_max_docid )
//     {
//       this_max_docid=  docid;
//     }
//     is_first = false;
//   }
//   if( this_max_docid >= min_docid) PairWiseCompare_(for_compare, min_docid);
//   for_compare.resize(0);
// }

uint32_t DupDetector2::GetProcessedMaxId_()
{
  return max_docid_;
}

bool DupDetector2::ProcessCollection()
{
  process_start_docid_ = GetProcessedMaxId_()+1;
  process_max_docid_ = document_manager_->getMaxDocId();
  if(process_max_docid_<process_start_docid_)
  {
    std::cout<<"No document need to processed: from "<<process_start_docid_<<" to "<<process_max_docid_<<std::endl;
    return true;
  }
  uint32_t total_count = process_max_docid_-process_start_docid_+1;
  std::cout<<"Will processing from "<<process_start_docid_<<" to "<<process_max_docid_<<std::endl;

  vec_.resize(total_count);
  uint32_t process_count = 0;
  Document doc;

  for( uint32_t docid = process_start_docid_; docid<=process_max_docid_; docid++)
  {
    process_count++;
    if( process_count %1000 == 0 )
    {
      MEMLOG("[DUPD] inserted %d. docid: %d", process_count, docid);
    }
    bool b = document_manager_->getDocument(docid, doc);
    if(!b) continue;
    Document::property_iterator property_it = doc.propertyBegin();
    
#ifdef DUPD_TEXT_DEBUG
    std::cout<<"["<<docid<<"] ";
#endif
    izenelib::util::CBitArray bit_array;
    uint32_t length = 0;
    while(property_it != doc.propertyEnd() )
    {
      if( dd_properties_.find( property_it->first)!= NULL)
      {
        std::vector<std::string> v;
        const izenelib::util::UString& content = property_it->second.get<izenelib::util::UString>();
        std::vector<izenelib::util::UString> termStrList;
        analyzer_->GetFilteredStringList( content, termStrList );
        for(uint32_t u=0;u<termStrList.size();u++)
        {
            if(termStrList[u].length()==1) continue;
            termStrList[u].toLowerString();
            std::string display;
            termStrList[u].convertString(display, izenelib::util::UString::UTF_8);
#ifdef DUPD_TEXT_DEBUG
            std::cout<<display<<",";
#endif
//             std::string str((const char*) termStrList[u].c_str(), termStrList[u].size());
            v.push_back(display);
        }
        if(!v.empty())
        {
            izenelib::util::CBitArray p_bit_array;
            //  CharikarAlgorithm algo;
            algo_->generate_document_signature(v, p_bit_array);
            if(bit_array.IsEmpty())
            {
                bit_array = p_bit_array;
            }
            else
            {
                bit_array^=p_bit_array;
            }
            length += v.size();
        //       std::cout<<"FP "<<docid<<" size "<<v.size()<<std::endl;
        }
      }
      property_it++;
    }
#ifdef DUPD_TEXT_DEBUG
    std::cout<<std::endl;
#endif
    if(!bit_array.IsEmpty())
    {
        vec_[docid-process_start_docid_] = FpItem(docid, bit_array, length);
    }
  }
  return runDuplicateDetectionAnalysis();
  
  
  //end
//   double sort_time = 0.0;
//   std::vector<std::pair<uint32_t, izenelib::util::CBitArray> > sorted_vec;
//   std::vector<std::pair<uint32_t, izenelib::util::CBitArray> > vec_all;
//   std::vector<std::pair<uint32_t, izenelib::util::CBitArray> >* data_vec = &vec_all;
//   for( uint32_t i=0; i<group_num;i++)
//   {
//     clocker.restart();
//     std::sort( vec.begin(), vec.end(), compare_obj_[i] );
//     if( boost::filesystem::exists( fp_groups_[i] ) )
//     {
//       ReaderType reader(fp_groups_[i]);
//       reader.open();
//       sorted_vec.resize(reader.getItemCount() );
//       uint32_t count = 0;
//       while( reader.next(docid, bitArray) )
//       {
//         sorted_vec[count] = std::make_pair(docid, bitArray);
//         count++;
//       }
//       reader.close();
//       vec_all.resize( sorted_vec.size()+vec.size() );
//       std::merge( sorted_vec.begin(), sorted_vec.end(), vec.begin(), vec.end(), vec_all.begin(), compare_obj_[i]);
//     }
//     else
//     {
//       sorted_vec.resize(0);
//       data_vec = &vec;
//     }
//     
//     sort_time += clocker.elapsed();
//     DataDupd(*data_vec, i, min_docid);
//     boost::filesystem::remove_all( fp_groups_[i]);
//     WriterType writer(fp_groups_[i]);
//     writer.open();
//     for( uint32_t j=0;j<(*data_vec).size(); j++)
//     {
//       writer.append( (*data_vec)[j].first, (*data_vec)[j].second);
//     }
//     writer.close();
//   }
//   
//   
//   std::cout<<"Sorting on fingerprints cost : "<<sort_time<<std::endl;
//   InitWriter_();
  
}

/**
 * Get a duplicated document list to a given document.
 * @return if no dupDocs for this docId, return false, else return false;
 */
bool DupDetector2::getDuplicatedDocIdList(uint32_t docId, std::vector<uint32_t>& docIdList)
{

  boost::lock_guard<boost::shared_mutex> lock(read_write_mutex_);
  group_->Find(docId, docIdList);
  if(!docIdList.empty())
  {
    for(uint32_t i=0;i<docIdList.size();i++)
    {
      if(docIdList[i]==docId)
      {
        docIdList.erase(docIdList.begin()+i);
        break;
      }
    }
  }
  return true;
}

/// Get a unique document id list with duplicated ones excluded.
bool DupDetector2::getUniqueDocIdList(const std::vector<uint32_t>& docIdList, std::vector<uint32_t>& cleanDocs)
{
  boost::lock_guard<boost::shared_mutex> lock(read_write_mutex_);
    ///Get all the dup docs of the collection.
  cleanDocs.assign(docIdList.begin(), docIdList.end());
  for (uint32_t i=0; i<cleanDocs.size(); ++i)
  {
    for (uint32_t j=i+1; j<cleanDocs.size(); ++j)
    {
        if (isDuplicated(cleanDocs[i], cleanDocs[j]))
        {
            cleanDocs.erase(cleanDocs.begin()+j);
            --j;
        }
    }
   
  }

  return true;
}

/// Tell two documents belonging to the same collections are duplicated.
bool DupDetector2::isDuplicated( uint32_t docId1, uint32_t docId2)
{
    return group_->IsSameGroup(docId1, docId2);
}

void DupDetector2::insertDocument(uint32_t docid, const std::vector<std::string>& v)
{
  if(docid<=vec_.size()&&docid>0)
  {
    izenelib::util::CBitArray bit_array;
    //  CharikarAlgorithm algo;
    algo_->generate_document_signature(v, bit_array);
    vec_[docid-1] = FpItem(docid, bit_array, v.size());
    if(process_start_docid_==0)
    {
      process_start_docid_ = docid;
    }
    if(docid>process_max_docid_)
    {
      process_max_docid_ = docid;
    }
  }
  else
  {
    std::cerr<<"not enough space for docid "<<docid<<std::endl;
  }
}
  
bool DupDetector2::runDuplicateDetectionAnalysis(bool force)
{
  if(!force && vec_.empty())
  {
    std::cout<<"No data for DD"<<std::endl;
    return true;
  }
  uint32_t table_count = table_list_.size();
  //only save one copy of all fps, sort it in every table time.
  std::vector<FpItem> processed;
  
  std::vector<FpItem> vec_all;
  fp_storage_.GetFpByTableId(0, vec_all);
  std::cout<<"Processed doc count : "<<vec_all.size()<<std::endl;
  vec_all.insert(vec_all.end(), vec_.begin(), vec_.end());
  vec_.clear();
  if(!fp_storage_.SetFpByTableId(0, vec_all))
  {
    std::cerr<<"Save fps failed"<<std::endl;
  }
  
#ifdef DUPD_FP_DEBUG
  for(uint32_t i=0;i<vec_all.size();i++)
  {
      const FpItem& item = vec_all[i];
      std::cout<<"FP "<<item.docid<<","<<item.length<<",";
      item.fp.display(std::cout);
      std::cout<<std::endl;
  }
#endif
  std::cout<<"Now all doc count : "<<vec_all.size()<<std::endl;
  std::cout<<"Table count : "<<table_count<<std::endl;
  
  for(uint32_t tid=0;tid<table_count;tid++)
  {
    MEMLOG("Processing table id : %d", tid);
    FpTable table = table_list_[tid];
    std::sort( vec_all.begin(), vec_all.end(), table );
    FindDD_(vec_all, table, tid);
  }
  
  uint32_t max = process_max_docid_;
  process_start_docid_ = 0;
  process_max_docid_ = 0;
  
  
  
  if(!group_->Flush())
  {
    std::cerr<<"group flush error"<<std::endl;
    return false;
  }
  max_docid_ = max;
  file_info_->SetValue(max_docid_);
  file_info_->Save();
  group_->PrintStat();
  return true;
}

// bool DupDetector2::insertDocument_(uint32_t docid, const std::vector<std::string >& term_list)
// {
//   
//     izenelib::util::CBitArray bitArray;
//     //  CharikarAlgorithm algo;
//     algo_->generate_document_signature(term_list, bitArray);
//     
// //     std::cout<<"[DUPD] "<<docid<<",";
// //     bitArray.display(std::cout);
// //     std::cout<<std::endl;
//     
//     writer_->append(docid, bitArray);
//     if( docid > max_docid_ ) max_docid_ = docid;
//     return true;
// 
// }
//  
// 
// /// Remove documents from a collection. From now, those documents should be excluded in the result.
// bool DupDetector2::removeDocument_(unsigned int docid)
// {
// //   if (!fp_index_->exist(docid))
// //     return false;
// //   
// //     read_write_mutex_.lock_shared();
// //     fp_index_->del_doc(docid);
// //     read_write_mutex_.unlock_shared();
//  return true;
// }


}


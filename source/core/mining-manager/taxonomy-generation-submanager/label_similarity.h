///
/// @file label_similarity.h
/// @brief label_similarity class
/// @author Jia Guo <guojia@gmail.com>
/// @date Created 2011-03-23
/// @date Updated 2011-03-23


#ifndef _SF1R_LABELSIMILARITY_H_
#define _SF1R_LABELSIMILARITY_H_

#include <string>
#include "../MiningManagerDef.h"

#include "TgTypes.h"
#include <util/error_handler.h>
#include <idmlib/util/file_object.h>
#include <boost/serialization/vector.hpp>
#include <idmlib/similarity/term_similarity.h>
namespace sf1r
{

class LabelSimilarity
{
  typedef std::pair<uint32_t, uint32_t> id2count_t;
public:
typedef idmlib::sim::TermSimilarityTable<uint32_t> SimTableType;
typedef idmlib::sim::SimOutputCollector<SimTableType> SimCollectorType;
typedef idmlib::sim::TermSimilarity<SimCollectorType> TermSimilarityType;
    LabelSimilarity(const std::string& path, const std::string& rig_path, boost::shared_ptr<SimCollectorType> collector)
    :sim_(new TermSimilarityType(path, rig_path, collector.get(), 0.4))
    {
    }
    
    ~LabelSimilarity()
    {
      delete sim_;
    }
                    
    bool Open(uint32_t context_max)
    {
      if(!sim_->Open()) return false;
      if(!sim_->SetContextMax(context_max)) return false;
      return true;
    }
    
        
    void Append(
    uint32_t label_id,
    const izenelib::util::UString& label_str,
    const std::vector<id2count_t>& doc_item_list, 
    uint8_t type,
    uint8_t score)
    {
      if(!sim_->Append(label_id, doc_item_list))
      {
        std::cerr<<"label similarity append "<<label_id<<" failed"<<std::endl;
      }
    }
    
    bool Compute()
    {
      return sim_->Compute();
    }

       
private:
    TermSimilarityType* sim_;
    
};
    
}

#endif

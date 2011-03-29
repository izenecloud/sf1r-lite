/**
   @file cn_correction_mgr.h
   @author Kevin Hu
   @date 2010.01.13
 */

#ifndef LOG_TABLE_H
#define LOG_TABLE_H

#include <ir/mr_word_id/term_freq.hpp>
#include <ir/mr_word_id/str_str_table.hpp>
#include "pinyin_dic.hpp"
#include <string>
#include <vector>
#include <util/ustring/UString.h>

namespace sf1r
{

  /**
     @class LogTable
   */ 
  class LogTable
  {
    
    std::string filenm_;
    izenelib::ir::StrStrTable<> pinyin2words_;
    izenelib::ir::TermFrequency<> word_freq_;
    
  public:

    LogTable(const char* filenm)
      :filenm_(filenm),pinyin2words_((filenm_+".pinyin").c_str())
    {
    }

    ~LogTable()
    {
      flush();
    }

    void flush()const;

    bool load();
    
    std::vector<std::string> get_correction(const std::string& query)const;

    void add_word(const std::string& pinyin, std::string word);
  }
  ;
  
}//namespace




#endif

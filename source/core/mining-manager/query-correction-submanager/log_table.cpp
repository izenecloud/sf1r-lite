#include "log_table.h"
#include <time.h>
#include <boost/bind.hpp>

namespace sf1r
{
  
  void LogTable::flush()const
  {
    FILE* f = fopen((filenm_+".wfreq").c_str(), "w+");
    word_freq_.save(f);
    fclose(f);

    pinyin2words_.flush();
  }
  

  bool LogTable::load()
  {
    FILE* f = fopen((filenm_+".wfreq").c_str(), "r");
    if (f == NULL)
      return false;
    
    word_freq_.load(f);
    fclose(f);

    return pinyin2words_.load();
  }
  
    
  std::vector<std::string>
  LogTable::get_correction(const std::string& pinyin)const
  {
    std::vector<std::string> r;
    
    std::string w = pinyin2words_.find(pinyin);
    uint32_t s = 0;
    for (uint32_t i=0; i<w.length(); ++i)
    {
      if (w[i] != ' ')
        continue;
      if (i-s<1)
        continue;
      
      r.push_back(w.substr(s, i-s));
      s = i+1;
    }

    uint32_t* freqs = new uint32_t[r.size()];
    for (uint32_t i=0; i<r.size(); ++i)
      freqs[i] = word_freq_.find(r[i]);
    
    uint32_t max_freq = 0;
    uint32_t max_i = -1;
    for (uint32_t i=0; i<r.size(); ++i)
      if (freqs[i]>max_freq)
      {
        max_freq = freqs[i];
        max_i = i;
      }

    std::vector<std::string> r1;
    if (max_i == (uint32_t)-1)
    {
      delete [] freqs;
      return r1;
    }

    r1.push_back(r[max_i]);
    for (uint32_t i=0; i<r.size(); ++i)
    {
      if (i!=max_i && (max_freq-freqs[i])/max_freq <= 0.1)
        r1.push_back(r[i]);
    }
    
    delete [] freqs;
    return r1;
  }

  void LogTable::add_word(const std::string& pinyin, std::string word)
  {
    word_freq_.increase(word);
    word += " ";
    pinyin2words_.insert(pinyin, word);
  }
   
}//namespace sf1r


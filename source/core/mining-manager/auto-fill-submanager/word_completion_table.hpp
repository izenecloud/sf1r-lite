/**
   @file word_competion_table.hpp
   @author Kevin Hu
   @date 2010.03.23
*/
#ifndef _WORD_COMPLETION_TABLE_HPP_
#define _WORD_COMPLETION_TABLE_HPP_

#include <types.h>
#include <boost/thread/mutex.hpp>
#include <boost/filesystem.hpp>
#include <stdio.h>
#include <string>
#include <am/dyn_array.hpp>
#include <am/external_sort/izene_sort.hpp>
#include "value_shared_table.h"
#include <algorithm>
#include <sdb/SequentialDB.h>
#include <util/ustring/UString.h>
#include <mining-manager/util/LabelDistance.h>
namespace sf1r
{

/**
 * @class WordCompletionTable
 **/
class WordCompletionTable
{
    typedef uint32_t DF_TYPE;
    typedef uint32_t FREQ_TYPE; 
    typedef uint8_t CHAR_TYPE;
    std::string dir_name_;
    izenelib::sdb::ordered_sdb<std::string, std::string, izenelib::util::ReadWriteLock>* sdb_;
    bool has_count_;
    izenelib::am::IzeneSort<CHAR_TYPE, uint32_t, true>* sorter_;
    char* buffer_;
    uint32_t bs_;
    uint32_t top_;

    struct FREQ_IDX
    {
        FREQ_TYPE freq_;
        uint32_t idx_;

        explicit FREQ_IDX()
        {
            freq_ = -1;
            idx_ = -1;
        }

        explicit FREQ_IDX(FREQ_TYPE freq, uint32_t idx)
        {
            freq_ = freq;
            idx_ = idx;
        }

        FREQ_IDX(const FREQ_IDX& other)
        {
            freq_ = other.freq_;
            idx_ = other.idx_;
        }

        FREQ_IDX& operator = (const FREQ_IDX& other)
        {
            freq_ = other.freq_;
            idx_ = other.idx_;
            return *this;
        }

        bool operator > (const FREQ_IDX& other )const
        {
            return freq_<other.freq_;
        }

        bool operator < (const FREQ_IDX& other )const
        {
            return freq_>other.freq_;
        }

        bool operator >= (const FREQ_IDX& other )const
        {
            return freq_<=other.freq_;
        }

        bool operator <= (const FREQ_IDX& other )const
        {
            return freq_>=other.freq_;
        }
    };

    inline void enlarge_buf_(uint32_t bs)
    {
        if (bs < bs_)
            return;
        buffer_ = (char*)realloc(buffer_, bs);
        bs_ = bs;
    }

    uint32_t get_key_len_(const char* buf)const
    {
        uint32_t len = 0;
        for (; ((CHAR_TYPE*)(buf))[len]!='\n'; ++len);
        return len;
    }

    uint32_t get_value_len_(const char* buf)const
    {
        uint32_t len = 0;
        for (; ((CHAR_TYPE*)(buf))[len]!='\n'; ++len);
        uint32_t r = 0;
        for (++len; ((CHAR_TYPE*)(buf))[len]!='\n'; ++len, ++r);
        return r;
    }
    
    DF_TYPE get_df_(const char* buf)const
    {
        uint32_t len = 0;
        for (; ((CHAR_TYPE*)(buf))[len]!='\n'; ++len);
        for (++len; ((CHAR_TYPE*)(buf))[len]!='\n'; ++len);
        ++len;
        return *(DF_TYPE*)(buf+len*sizeof(CHAR_TYPE));
    }

    FREQ_TYPE get_freq_(const char* buf)const
    {
        uint32_t len = 0;
        for (; ((CHAR_TYPE*)(buf))[len]!='\n'; ++len);
        for (++len; ((CHAR_TYPE*)(buf))[len]!='\n'; ++len);
        ++len;
        return *(FREQ_TYPE*)(buf+len*sizeof(CHAR_TYPE)+sizeof(DF_TYPE));
    }

    inline void pack_up_(uint32_t start, uint32_t end, uint32_t n, const std::vector<uint32_t>& lens)
    {
        if (start == lens.size() || start == end)
            return;

        while (start<end && start<lens.size()
                && get_key_len_(buffer_+lens[start]) < n)
            ++start;

        if (start == end || start==lens.size())
            return;

        std::string cmp(buffer_+lens[start], n*sizeof(CHAR_TYPE));

        std::vector<FREQ_IDX> list;
        list.push_back(FREQ_IDX(get_freq_(buffer_+lens[start]), start));

        uint32_t i = start+1;
        for (; i<lens.size() && i<end
                && cmp == std::string(buffer_+lens[i], n*sizeof(CHAR_TYPE));
                ++i)
            //push to list
            list.push_back(FREQ_IDX(get_freq_(buffer_+lens[i]), i));

        //sort and add into vst
        std::sort(list.begin(), list.end());
        std::string strlist = "";
        uint32_t idx = -1;
        for (uint32_t j=0; j<list.size() && j<top_; ++j)
        {
            assert(list[j].idx_ != idx);
            idx = list[j].idx_;
            //buf includes key,\n,value,\n,df,freq
            char* buf = buffer_ + lens[list[j].idx_];
            //starts with value, ends before freq
            strlist += std::string( 
            buf + (get_key_len_(buf)+1)*sizeof(CHAR_TYPE),
            (get_value_len_(buf)+1)*sizeof(CHAR_TYPE)+sizeof(DF_TYPE)
            );

//         std::cout<<std::string( buf, get_key_len_(buf)*sizeof(CHAR_TYPE))
//                  <<":"
//                  <<std::string( buf + (get_key_len_(buf)+1)*sizeof(CHAR_TYPE),
//                                 (get_value_len_(buf))*sizeof(CHAR_TYPE))
//                  <<std::endl;

        }

        //vst_->put(cmp, strlist);
        sdb_->insert(cmp, strlist);

        pack_up_(start, i, n+1, lens);
        assert(i > start);
        pack_up_(i, end, n, lens);
    }

public:
    WordCompletionTable(const std::string& dir_nm, uint32_t top = 10, uint32_t max_buf = 100000000)
            :dir_name_(dir_nm), sdb_(NULL), has_count_(false), sorter_(NULL),
            buffer_(NULL), bs_(0), top_(top)
    {
        if (!boost::filesystem::is_directory(dir_name_))
        {
            try
            {
                boost::filesystem::create_directories(dir_name_);
            }
            catch ( boost::filesystem::filesystem_error& e )
            {
                throw e;
                return;
            }
        } // end - try-catch
        
        if (boost::filesystem::exists(dir_name_+"/has_count"))
        {
            has_count_ = true;
        }
        sdb_  = new izenelib::sdb::ordered_sdb<std::string, std::string, izenelib::util::ReadWriteLock>(dir_name_+"/table");
        sdb_->setCacheSize(30000);
        sdb_->open();
        sdb_->getContainer().fillCache();
        //std::cout<<"vst number: "<<vst_->item_num()<<std::endl;
    }

    ~WordCompletionTable()
    {
        if (buffer_)
            free(buffer_);
        if (sorter_)
            delete sorter_;
        // if (vst_)
//         delete vst_;
        if (sdb_)
            delete sdb_;
    }

    void set_top(uint32_t top)
    {
        top_=top;
    }

    void add_item(const std::string& key, const std::string& value, DF_TYPE df, FREQ_TYPE freq)
    {
        if (!sdb_)
            return;

        if (!sorter_)
        {
            sorter_ = new izenelib::am::IzeneSort<CHAR_TYPE, uint32_t, true>((dir_name_+"/sort").c_str());
        }

        uint32_t len = key.length() + value.length() + sizeof(df) + sizeof(freq) + 2*sizeof(CHAR_TYPE);
        enlarge_buf_(len);
        memcpy(buffer_, key.c_str(), key.length());
        *(CHAR_TYPE*)(buffer_+key.length()) = '\n';
        memcpy(buffer_+key.length()+sizeof(CHAR_TYPE), value.c_str(), value.length());
        *(CHAR_TYPE*)(buffer_+key.length() + sizeof(CHAR_TYPE) + value.length() ) = '\n';
        //df first, then freq.
        *(FREQ_TYPE*)(buffer_+key.length() + sizeof(CHAR_TYPE) + value.length() + sizeof(CHAR_TYPE) ) = df;
        *(FREQ_TYPE*)(buffer_+key.length() + sizeof(CHAR_TYPE) + value.length() + sizeof(CHAR_TYPE) + sizeof(DF_TYPE) ) = freq;

        sorter_->add_data(len, buffer_);
    }

    void add_item(const izenelib::util::UString& key, const izenelib::util::UString& value, DF_TYPE df, FREQ_TYPE freq)
    {
        std::string s1 = "";
        key.convertString(s1, izenelib::util::UString::UTF_8);
        std::string s2 = "";
        value.convertString(s2, izenelib::util::UString::UTF_8);
        add_item(s1, s2, df, freq);
        //sstd::cout<<s1<<":"<<s2<<std::endl;
    }


    bool get_item(const izenelib::util::UString& query, std::vector<std::pair<izenelib::util::UString, uint32_t> >& list)
    {
        std::string value = "";
        std::string key = "";
        query.convertString(key, izenelib::util::UString::UTF_8);
        sdb_->get(key, value);
        //std::cout<<key<<":"<<value<<std::endl;

        list.clear();
        uint32_t s = 0;
        const char* p = value.c_str();
        std::vector<izenelib::util::UString> text_list;
        for (uint32_t i=0; i<value.length(); ++i)
        {
            if (value[i]=='\n')
            {
                izenelib::util::UString str(value.substr(s, i-s), izenelib::util::UString::UTF_8);
                bool flag = false;
                for (uint32_t j=0; j<text_list.size(); ++j)
                {
                    if (text_list[j] == str)
                    {
                        flag = true;
                        break;
                    }
                }
                if (!flag&&!LabelDistance::isDuplicate(str, text_list))
                {
                    DF_TYPE df = 0;
                    if(has_count_)
                    {
                        const char* pc = p+sizeof(CHAR_TYPE)*(i+1);
                        const DF_TYPE* dp = (const DF_TYPE*)pc;
                        df = *dp;
                    }
                    list.push_back(std::make_pair(str, df));
                    text_list.push_back(str);
//                     std::cout<<"[autofill] "<<str<<","<<df<<std::endl;
                }
                if(has_count_)
                {
                    i += sizeof(DF_TYPE);
                }
                s = i+1;
            }
        }

        if (list.size())
            return true;
        return false;
    }

    void flush()
    {
        if (!sorter_)
            return;

        sorter_->sort();

        enlarge_buf_(10000000);
        uint32_t buf_pos = 0;

        char* buf = NULL;
        if (!sorter_->begin())
            return;

        std::vector<uint32_t> lens;
        uint32_t len;
        CHAR_TYPE first = -1;
        std::string last_str = "";
        uint32_t i = 0;
        std::cout<<"autofill begin flushing.."<<std::endl;
        while (sorter_->next_data(len, &buf))
        {
            ++i;
//         std::cout<<"\r"<<1.0*i/sorter_->item_num()*100.<<"%"<<std::flush;
            if (first!=(CHAR_TYPE)-1 && first!=*(CHAR_TYPE*)buf)
            {
                pack_up_(0, lens.size(), 1, lens);
                buf_pos = 0;
                lens.clear();
            }

            first = *(CHAR_TYPE*)buf;
            //tmp include key,\n,value,\n
            std::string tmp(buf, len-sizeof(FREQ_TYPE)-sizeof(DF_TYPE));
            if (last_str.compare(tmp) == 0)
            {
                FREQ_TYPE max_freq = std::max( *(FREQ_TYPE*)(buffer_+buf_pos-sizeof(FREQ_TYPE)), *(FREQ_TYPE*)(buf+len-sizeof(FREQ_TYPE)) );
                *(FREQ_TYPE*)(buffer_+buf_pos-sizeof(FREQ_TYPE)) = max_freq;
                free(buf);
                continue;
            }
            last_str = tmp;
            //std::cout<<tmp<<std::endl;

            enlarge_buf_(buf_pos+len);
            memcpy(buffer_+buf_pos, buf, len);
            last_str = tmp;

//         if (buf[0] == 'z'&& buf[7] == 'o')
//         std::cout<<std::string( buffer_+buf_pos, get_key_len_(buffer_+buf_pos)*sizeof(CHAR_TYPE))
//                  <<":"
//                  <<std::string( buffer_+buf_pos + (get_key_len_(buffer_+buf_pos)+1)*sizeof(CHAR_TYPE),
//                                  (get_value_len_(buffer_+buf_pos))*sizeof(CHAR_TYPE))
//                  <<std::endl;

            lens.push_back(buf_pos);
            buf_pos += len;
            free(buf);
        }
        std::cout<<"autofill finish flushing."<<std::endl;
        if (buf_pos)
            pack_up_(0, lens.size(), 1, lens);

        sorter_->clear_files();
        delete sorter_;
        sorter_ = NULL;

        //vst_->flush();
        sdb_->flush();
        has_count_ = true;
        std::string has_count_file = dir_name_+"/has_count";
        std::ofstream ofs(has_count_file.c_str());
        ofs<<"a";
        ofs.close();
    }

}
;
}
#endif

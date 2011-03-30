/**
   @file pinyin_dic.hpp
   @auther Kevin Hu
   @date 2010.01.09
 **/

#ifndef PINYIN_DIC_HPP
#define PINYIN_DIC_HPP

#include <ir/mr_word_id/str_str_table.hpp>
#include <string>
#include <vector>
#include <util/ustring/UString.h>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

namespace sf1r
{

template<
izenelib::util::UString::EncodingType ENCODE_TYPE = izenelib::util::UString::UTF_8
>
class PinyinDictionary
{
    typedef izenelib::ir::StrStrTable<ENCODE_TYPE> table_t;


    table_t pinyin2word_table_;
    table_t word2pinyin_table_;
    std::string filenm_;

    inline void parse_line_(const std::string& line, std::string& word, std::string& pinyin)
    {
        //std::cout<<line<<std::endl;

        word = pinyin = "";
        IASSERT (line.length()>0);

        std::size_t i = line.find(" ");
        IASSERT(i!= (std::size_t)-1 && i>0 && i<line.length()-1);
        word = line.substr(0, i);

        uint32_t j=i+1;
        for (; j<line.length() && (line[j]>='a'&& line[j]<='z'); ++j )
            ;

        pinyin = line.substr(i+1, j-i-1);
    }

    bool init_pinyin_tables_(const char* dicname)
    {
        FILE* f = fopen(dicname, "r");
        //FILE* ff = fopen("cn_dict.txt", "w+");
        if (f == NULL)
            return false;

        fseek(f, 0, SEEK_END);
        uint64_t fs = ftell(f);
        fseek(f, 0, SEEK_SET);
        if (fs == 0)
            return false;

        uint64_t pos_s = 0;
        uint64_t pos_e = 0;

        char* buff = new char[fs];
        IASSERT(fread(buff, fs, 1, f)==1);
        fclose(f);

        std::string word, pinyin;
        std::string last;
        while (pos_s < fs && pos_e<=fs)
        {
            if (buff[pos_e]!='\n')
            {
                ++pos_e;
                continue;
            }

            if (pos_e - pos_s <=1)
            {
                ++pos_e;
                pos_s = pos_e;
                continue;
            }

            std::string line(&buff[pos_s], pos_e - pos_s);
            parse_line_(line, word, pinyin);

            pinyin2word_table_.insert(pinyin, word+" ");
            word2pinyin_table_.insert(word, pinyin+" ");

            // if (word.compare(last)!=0)
//       {
//         last = word;
//         word += '\n';
//         fwrite(word.c_str(), word.length(), 1, ff);
//       }

            ++pos_e;
            pos_s = pos_e;
        }

        delete buff;
        //fclose(ff);
        return true;
    }

    void save_()
    {
        pinyin2word_table_.flush();
        word2pinyin_table_.flush();
    }

    bool load_()
    {
        return (
                   pinyin2word_table_.load() &&
                   word2pinyin_table_.load());

    }

public:
    PinyinDictionary(const std::string& filenm)
            :pinyin2word_table_((filenm+".pinyin2word").c_str()),
            word2pinyin_table_((filenm+".word2pinyin").c_str())
    {
    }

    ~PinyinDictionary()
    {
    }

    bool is_ready()
    {
        return load_();
    }

    bool prepare(const char* dicname)
    {
        if (!init_pinyin_tables_(dicname))
            return false;
        save_();
        return true;
    }

    std::vector<std::string> get_pinyins(const std::string& word )const
    {
        std::vector<std::string> r;
        std::string str = word2pinyin_table_.find(word);
        uint32_t s = 0;
        for (uint32_t i=0; i<str.length(); ++i)
        {
            if (str[i]!=' ')
                continue;
            r.push_back(str.substr(s, i-s));
            s = i+1;
        }

        return r;
    }

    std::vector<std::string> get_words(const std::string& pinyin )const
    {
        std::vector<std::string> r;
        std::string str = pinyin2word_table_.find(pinyin);
        uint32_t s = 0;
        for (uint32_t i=0; i<str.length(); ++i)
        {
            if (str[i]!=' ')
                continue;
            r.push_back(str.substr(s, i-s));
            s = i+1;
        }

        return r;
    }

    uint32_t word_items()const
    {
        return word2pinyin_table_.num_items();
    }

    uint32_t pinyin_items()const
    {
        return pinyin2word_table_.num_items();
    }

}
;

}//namespace

#endif//MISTER_WORDID_H

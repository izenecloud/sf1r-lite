#ifndef SF1R_MINING_MANAGER_SUMMARIZATION_OPINIONTRAINING_H
#define SF1R_MINING_MANAGER_SUMMARIZATION_OPINIONTRAINING_H

#include <string.h>
//#include <icma/icma.h>
#include <boost/tuple/tuple.hpp>
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/filesystem.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <iostream>
#include <mining-manager/auto-fill-submanager/word_leveldb_table.hpp>
#include <util/ustring/UString.h>

using namespace std;

namespace sf1r
{


namespace inner
{
    static const UCS2Char  en_dou(','), en_ju('.'), en_gantan('!'),
                 en_wen('?'), en_mao(':'), en_space(' ');
    //static const UCS2Char  ch_ju('。'), ch_dou('，'), ch_gantan('！'), ch_wen('？');
    static const UCS2Char  ch_ju(0x3002), ch_dou(0xff0c), ch_gantan(0xff01), ch_wen(0xff1f), ch_mao(0xff1a), ch_dun(0x3001);
}

class OpinionTraining
{
    typedef Table<std::string, int> LevelDBType;
    LevelDBType dbTable_OpinionTraining_begin;
    LevelDBType dbTable_OpinionTraining_end;

    size_t HMMmodleSize;
    fstream outlog;
    fstream total_size_file;
    vector<string>  example;
    string scdPath;
    bool need_rebuild;
public:

    OpinionTraining(string collectionpath)
    {
        string leveldbpath = collectionpath + "/OpinionTraining";
        string begin_leveldb = leveldbpath + "-begin";
        string end_leveldb = leveldbpath + "-end";
        string path = collectionpath + "/OpinionTraining.log";
        need_rebuild = true;
        try{
        if(boost::filesystem::exists(path) && 
            boost::filesystem::exists(begin_leveldb) &&
            boost::filesystem::exists(end_leveldb) )
        {
            need_rebuild = false;
        }
        else
        {
            boost::filesystem::remove_all(path);
            boost::filesystem::remove_all(begin_leveldb);
            boost::filesystem::remove_all(end_leveldb);
        }
        } catch (...) {}

        dbTable_OpinionTraining_begin.open(begin_leveldb);
        dbTable_OpinionTraining_end.open(end_leveldb);
        scdPath = collectionpath;
        HMMmodleSize = 3;
        if(need_rebuild)
            outlog.open(path.c_str(), ios::out);
        else
        {
            outlog.open(path.c_str(), ios::app);
        }
    }
    ~OpinionTraining()
    {
        dbTable_OpinionTraining_begin.close();
        dbTable_OpinionTraining_end.close();
        outlog.close();
    }
    void GetAndCompute(const std::string& content)
    {
        UString ucontent(content, UString::UTF_8);
        GetAndCompute(ucontent);
    }
    void GetAndCompute(const UString& ucontent)
    {
        size_t start = 0;
        size_t end = 0;
        while( end < ucontent.length() )
        {   
            if(IsCommentSplitChar(ucontent[end]))
            {
                UString sub_sentence = ucontent.substr(start, end - start);
                computeString( sub_sentence );
                start = end + 1;
            }
            ++end;
        }
        if(start < end)
        {
            computeString(ucontent.substr(start, end - start));
        }
    }

    void AppendSentence(const std::vector<std::string>& orig_text)
    {
        for(size_t i = 0; i < orig_text.size(); ++i)
        {   
            GetAndCompute(orig_text[i]);
        }
    }
    void AppendSentence(const std::vector<UString>& orig_text)
    {
        for(size_t i = 0; i < orig_text.size(); ++i)
        {   
            GetAndCompute(orig_text[i]);
        }
    }

    void LoadFile()
    {
        if(!need_rebuild)
        {
            return;
        }
        std::cout << "start rebuild opinion training data" << endl;
        uint32_t count = 0;
        boost::filesystem::path p(scdPath);
        //scdPath = scdPath + "/B-00-201207171255-46229-I-C.SCD";
        boost::filesystem::directory_iterator d_it = boost::filesystem::directory_iterator(p);
        vector<string> result;
        while(d_it != boost::filesystem::directory_iterator())
        {
            if(!boost::filesystem::is_regular_file((*d_it)))
            {
                ++d_it;
                continue;
            }
            ifstream in;
            string filename = (*d_it).path().c_str();
            if(filename.find(".SCD") != filename.size() - 4)
            {
                ++d_it;
                continue;
            }
            in.open(filename.c_str(), ios::in);
            vector<string> exampleTemp; 
            if (in.good())
            {
                std::cout << "begin processing scd file: " << filename << std::endl;
                string Content="<Content>";
                string Advantage="<Advantage>";
                string Disadvantage="<Disadvantage>";

                while(in.good())
                {   
                    std::string temp;
                    getline(in, temp);
                    if(temp.find(Content) != std::string::npos)
                    {
                        count++;
                        GetAndCompute(temp.substr(Content.size()));
                    }
                    else if(temp.find(Advantage) != std::string::npos)
                    {
                        count++;
                        GetAndCompute(temp.substr(Advantage.size()));
                    }
                    else if(temp.find(Disadvantage) != std::string::npos)
                    {   
                        count++;
                        GetAndCompute(temp.substr(Disadvantage.size()));
                    }
                    if(count % 10000 == 0)
                    {
                        std::cout << "\r == training processed " << count << " doc. ==" << std::flush;
                    }
                    if(count > 10000000)
                        break;
                }
                in.close();
            }
            else
            {
                in.close();
            }

            ++d_it;
        }

        need_rebuild = false;
        outlog.flush();
        dbTable_OpinionTraining_begin.flush();
        dbTable_OpinionTraining_end.flush();
    }

    void computeString(const UString& ustr)
    {   
        // get the start and the end bigram of a sentence.
        // the others can determine whether a sentence is a valid.
        if(ustr.length() < 2)
            return;
        size_t len = ustr.length();
        string beginstr = boost::lexical_cast<std::string>(ustr[0])
            + boost::lexical_cast<std::string>(ustr[1]);
        string endstr = boost::lexical_cast<std::string>(ustr[len - 2])
            + boost::lexical_cast<std::string>(ustr[len - 1]);
        int HitNum = 1;
        //outlog << "beginstr: " << beginstr << ", endstr: " << endstr << endl;
        //UString bigram_ustr = ustr.substr(0, 2);
        //string bigram_str;
        //bigram_ustr.convertString(bigram_str, UString::UTF_8);
        //outlog << "begin:" << bigram_str << endl;
        //if(bigram_str.find("的") == 0)
        //{
        //    string orig_str;
        //    ustr.convertString(orig_str, UString::UTF_8);
        //    outlog << " orig: " << orig_str << endl;
        //}
        if(dbTable_OpinionTraining_begin.get(beginstr, HitNum))
        {
            HitNum++;
        }
        dbTable_OpinionTraining_begin.insert(beginstr, HitNum);

        HitNum = 1;
        if(dbTable_OpinionTraining_end.get(endstr, HitNum))
        {
            HitNum++;
        }
        dbTable_OpinionTraining_end.insert(endstr, HitNum);
    }

    int Freq_Begin(const UString& ustr)
    {
        if(ustr.length() < 2)
            return 0;
        string beginstr = boost::lexical_cast<std::string>(ustr[0])
            + boost::lexical_cast<std::string>(ustr[1]);
        return Freq_Begin(beginstr);
    }
    int Freq_Begin(const string& str)
    {     
        int HitNum;
        if(dbTable_OpinionTraining_begin.get(str, HitNum))
        {    
            return HitNum;
        } 
        return 0;
    }
    int Freq_End(const UString& ustr)
    {
        if(ustr.length() < 2)
            return 0;
        string endstr = boost::lexical_cast<std::string>(ustr[ustr.length() - 2])
            + boost::lexical_cast<std::string>(ustr[ustr.length() - 1]);
        return Freq_End(endstr);
    }

    int Freq_End(const string& str)
    {     
        int HitNum;
        if(dbTable_OpinionTraining_end.get(str, HitNum))
        {    
            return HitNum;
        } 

        return 0;
    }


    static inline bool IsFullWidthChar(UCS2Char c)
    {
        return c >= 0xff00 && c <= 0xffef;
    }

    static inline bool IsCJKSymbols(UCS2Char c)
    {
        return c >= 0x3000 && c <= 0x303f;
    }

    static inline bool IsCommentSplitChar(UCS2Char c)
    {
        return c == inner::en_dou || c == inner::en_ju || c == inner::en_gantan || 
            c == inner::en_wen || c == inner::en_mao || c == inner::en_space ||
            IsFullWidthChar(c) || IsCJKSymbols(c);
    }

    static inline bool IsNeedIgnoreChar(UCS2Char c)
    {
        if(IsCommentSplitChar(c))
            return false;
        return !izenelib::util::UString::isThisChineseChar(c) &&
            !izenelib::util::UString::isThisDigitChar(c) &&
            !izenelib::util::UString::isThisAlphaChar(c);
    }

    static inline bool IsCommentSplitStr(const UString& ustr)
    {
        for(size_t i = 0; i < ustr.length(); ++i)
        {
            if(IsCommentSplitChar(ustr[i]))
                return true;
        }
        return false;
    }

    static inline bool IsAllChineseStr(const UString& ustr)
    {
        for(size_t i = 0; i < ustr.length(); ++i)
        {
            if(!izenelib::util::UString::isThisChineseChar(ustr[i]))
                return false;
        }
        return true;
    }

};

}
#endif

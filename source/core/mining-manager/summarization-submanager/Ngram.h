#ifndef SF1R_MINING_MANAGER_SUMMARIZATION_NGRAM_H
#define SF1R_MINING_MANAGER_SUMMARIZATION_NGRAM_H

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

    //bool isThisChineseChar(UCS2Char ucs2Char)
    //{
    //    if ((ucs2Char>=0x2E80 && ucs2Char<=0x2EF3)
    //        || (ucs2Char>=0x2F00 && ucs2Char<=0x2FD5)
    //        || (ucs2Char>=0x3400 && ucs2Char<=0x4DB5)
    //        || (ucs2Char>=0x4E00 && ucs2Char<=0x9FC3)
    //        || (ucs2Char>=0xF900 && ucs2Char<=0xFAD9))
    //        return true;

    //    return false;
    //} // end - isThisChineseChar()

    inline bool IsNotChineseChar(UCS2Char c)
    {
        return !izenelib::util::UString::isThisChineseChar(c);
    }


class Ngram
{
    typedef Table<izenelib::util::UString, int> LevelDBType;
    LevelDBType dbTable_Ngram;

    size_t HMMmodleSize;
    fstream outlog;
    fstream total_size_file;
    vector<string>  example;
    string scdPath;
    bool need_rebuild;
public:

    Ngram(string collectionpath)
    {
        string leveldbpath = collectionpath + "/Ngram";
        string path = collectionpath + "/Ngram.log";
        need_rebuild = true;
        try{
        if(boost::filesystem::exists(path) && boost::filesystem::exists(leveldbpath))
        {
            need_rebuild = false;
        }
        else
        {
            boost::filesystem::remove_all(path);
            boost::filesystem::remove_all(leveldbpath);
        }
        } catch (...) {}
        dbTable_Ngram.open(leveldbpath);
        scdPath = collectionpath;
        HMMmodleSize = 3;
        outlog.open(path.c_str(), ios::out);
    };
    ~Ngram()
    {
        dbTable_Ngram.close();
        outlog.close();
    };
    void GetAndCompute(const std::string& content)
    {
        string temp = content;
        string dot="，";
        size_t templen = 0; 
        while(!temp.empty())
        {   
            size_t len1 = temp.find("，");
            size_t len2 = temp.find("。");
            if(len1 != string::npos || len2 != string::npos)
            {
                if(len1 == string::npos)
                {
                    templen = len2;
                }
                else if(len2 == string::npos)
                {
                    templen = len1;
                }
                else
                {
                    templen = min(len1,len2);
                }
                computeString( temp.substr(templen) );
                temp = temp.substr(templen + dot.length(), temp.size() - templen + dot.length());
            }
            else
            {
                computeString(temp);
                break;
            }
        }
    }

    void AppendSentence(std::vector<std::string> orig_text)
    {
        for(size_t i = 0; i < orig_text.size(); ++i)
        {   
            const std::string& temp = orig_text[i];
            GetAndCompute(temp);
        }
    }

    void LoadFile()
    {
        if(!need_rebuild)
        {
            std::cout << "rebuild is not needed" << std::endl;
            return;
        }
        outlog << "start rebuild ngram" << endl;
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
            //out<<*it<<endl;
            string filename = (*d_it).path().c_str();
            if(filename.find(".SCD") != filename.size() - 4)
            {
                ++d_it;
                continue;
            }
            in.open(filename.c_str(), ios::in);
            //out<<"open scd"<<endl;
            int IntegrationNumber=0;
            vector<string> exampleTemp; 
            if (in.good())
            {
                std::cout << "begin processing scd file: " << filename << std::endl;
                string Title="<Title>";
                string Content="<Content>";
                //string ProdName="<ProdName>";
                string Integration="<Integration>";

                while(in.good())
                {   
                    std::string temp;
                    getline(in, temp);
                    if(temp.find(Title) != std::string::npos)
                    {
                        GetAndCompute(temp.substr(Title.size()));
                    }
                    else if(temp.find(Content) != std::string::npos)
                    {
                        GetAndCompute(temp.substr(Content.size()));
                    }
                    //else if(temp.find(ProdName) != std::string::npos)
                    //{   
                    //    GetAndCompute(temp.substr(ProdName.size()));
                    //}
                    else if(temp.find(Integration) != std::string::npos)
                    {   
                        IntegrationNumber++;
                        if( IntegrationNumber<100)
                        {
                            result.push_back(temp);
                        }
                        GetAndCompute(temp.substr(Integration.size()));
                    }
                    count++;
                    if(count % 5000 == 0)
                    {
                        std::cout << "ngram processed " << count << " doc." << std::endl;
                    }
                    if(count > 900000)
                    {
                        break;
                    }
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
        for(size_t i = 0; i < result.size(); i++)
        { 
            outlog << result[i] << endl;
        }
        example = result;
        outlog.flush();
        dbTable_Ngram.flush();
    }

    vector<string> getExample()
    {
        return example;
    }
    void computeString(const string& str)
    {   
        izenelib::util::UString ustr(str, izenelib::util::UString::UTF_8);
        izenelib::util::UString::iterator uend = std::remove_if(ustr.begin(), ustr.end(), IsNotChineseChar);
        ustr.assign(ustr.begin(), uend);

        // outlog<<"computeString1"<<endl;
        string strseg;
        int HitNum;
        size_t len = ustr.length();
        for(size_t i = 0; i < len; i++)
        {   
            //outlog<<"computeString2"<<endl;
            size_t compute_times = min(HMMmodleSize, len - i);
            for(size_t k = 1; k <= compute_times; k++)
            {
                //outlog<<"i"<<i<<"      k"<<k<<endl;
                izenelib::util::UString ustrseg = ustr.substr(i, k);
                ustrseg.convertString(strseg, UString::UTF_8);
                outlog<<"strseg         "<<strseg<<endl;
                if(dbTable_Ngram.get(strseg, HitNum))
                {
                    HitNum++;
                }
                else
                {
                    HitNum = 1;
                }
                dbTable_Ngram.insert(strseg, HitNum);
            }
            //outlog<<"computeString3"<<endl;
        }
    }

    double Freq(const string& str)
    {     
        int HitNum;
        if(dbTable_Ngram.get(str, HitNum))
        {    
            return double(HitNum);
        } 

        return 0;
    }

    double Possiblity(const string& str, const string& str1)
    {   
        if(Freq(str1) != 0)
        {
            return Freq(str1 + str)/Freq(str1);
        }
        return 0;
    }

    double Possiblity(const string& str, const string& str1, const string& str2)
    {
        if(Freq(str2 + str1 + str) == 0)
        {
            //back-off
            return Possiblity(str1, str2)*0.01;
        }
        assert(Freq(str2 + str1) > 0);
        return Freq(str2 + str1 + str)/Freq(str2 + str1);
    }

    void test()
    {     
        // outlog<<"computeString"<<endl;

        outlog<<"Possiblity"<<endl;
        //outlog<<"Possiblity(要)"<<Possiblity("要")<<endl;
        //outlog<<"Possiblity(个人)"<<Possiblity("个人")<<endl;
        //outlog<<"Possiblity(效果)"<<Possiblity("效果")<<endl;
        //outlog<<"Possiblity(价格)"<<Possiblity("价格")<<endl;
        outlog<<"Possiblity(风，格)"<<Possiblity("风","格")<<endl;
        outlog<<"Possiblity(黄,伟,文)"<<Possiblity("黄","伟","文")<<endl;
        outlog<<"Possiblity(伟,黄)"<<Possiblity("伟","黄")<<endl;
        outlog<<"finish"<<endl;
    }

};

}
#endif

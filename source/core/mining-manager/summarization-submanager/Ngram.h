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
#include <mining-manager/auto-fill-submanager/word_leveldb_table.hpp>
using namespace std;
namespace sf1r
{

class Ngram
{
    typedef Table<std::string, int> LevelDBType;
    LevelDBType dbTable_Ngram;

    int HMMmodleSize;
    fstream outlog;
    vector<string>  example;
    string scdPath;
public:

    Ngram(string collectionpath)
    {
        string leveldbpath = collectionpath + "/Ngram";
        dbTable_Ngram.open(leveldbpath);
        string path = collectionpath + "/Ngram.log";
        scdPath = collectionpath + "/B-00-201207171255-46229-I-C.SCD";
        outlog.open(path.c_str(), ios::out);
        HMMmodleSize = 3;
        outlog << "start ngram" << endl;
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

    void LoadFile()
    {
        string filePath=scdPath;
        ifstream in;
        //out<<*it<<endl;
        string filename = filePath;
        in.open(filename.c_str(), ios::in);
        //out<<"open scd"<<endl;
        int IntegrationNumber=0;
        vector<string> exampleTemp; 
        vector<string> result;
        if (in.good())
        {
            //out<<"in good"<<endl;
            std::string temp;
            string Title="<Title>";
            string Content="<Content>";
            string ProdName="<ProdName>";
            string Integration="<Integration>";
            uint32_t count = 0;

            while(count < 100000)
            {   
                outlog<<count<<endl;
                getline(in, temp);
                if(temp.substr(0,Title.size())==Title)
                {
                    temp = temp.substr(Title.size());
                    GetAndCompute(temp);
                }
                else if(temp.substr(0,Content.size())==Content)
                {
                    temp = temp.substr(Content.size());
                    GetAndCompute(temp);
                }
                else if(temp.substr(0,ProdName.size())==ProdName)
                {   
                    temp=temp.substr(ProdName.size());
                    GetAndCompute(temp);
                }
                else if(temp.substr(0,Integration.size())==Integration)
                {   
                    temp=temp.substr(Integration.size());
                    IntegrationNumber++;
                    if( IntegrationNumber<100)
                    {
                        result.push_back(temp);
                    }
                    GetAndCompute(temp);
                }
                count++;
            }
            in.close();
        }
        else
        {
            in.close();
        }
        for(size_t i = 0; i < result.size(); i++)
        { 
            outlog << result[i] << endl;
        }
        example = result;
    }

    vector<string> getExample()
    {
        return example;
    }

    void computeString(string str)
    {   
        izenelib::util::UString ustr(str, izenelib::util::UString::UTF_8);
        // outlog<<"computeString1"<<endl;
        string strseg;
        int HitNum;
        for(size_t i = 0; i < ustr.length(); i++)
        {   
            //outlog<<"computeString2"<<endl;
            for(size_t k = max(0, (int)i - HMMmodleSize + 1); k <= i; k++)
            {
                //outlog<<"i"<<i<<"      k"<<k<<endl;
                izenelib::util::UString ustrseg = ustr.substr(k, i - k + 1);
                ustrseg.convertString(strseg, UString::UTF_8);
                //outlog<<"strseg         "<<strseg<<endl;
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

    double Freq(string str)
    {     
        int HitNum;
        if(dbTable_Ngram.get(str, HitNum))
        {    
            return double(HitNum);
        } 

        return 0;
    }

    double Possiblity(string str)
    {
        return Freq(str);
    }
    double Possiblity(string str, string str1)
    {   
        if(Freq(str1) != 0)
        {
            return Freq(str1 + str)/Freq(str1);
        }
        return 0;
    }
    double Possiblity(string str, string str1, string str2)
    {
        if(Freq(str2 + str1) != 0)
        {
            return Freq(str2 + str1 + str)/Freq(str2 + str1);
        }
        return 0;
    }

    void test()
    {     
        // outlog<<"computeString"<<endl;

        outlog<<"Possiblity"<<endl;
        outlog<<"Possiblity(要)"<<Possiblity("要")<<endl;
        outlog<<"Possiblity(个人)"<<Possiblity("个人")<<endl;
        outlog<<"Possiblity(效果)"<<Possiblity("效果")<<endl;
        outlog<<"Possiblity(价格)"<<Possiblity("价格")<<endl;
        outlog<<"Possiblity(风，格)"<<Possiblity("风","格")<<endl;
        outlog<<"Possiblity(黄,伟,文)"<<Possiblity("黄","伟","文")<<endl;
        outlog<<"Possiblity(伟,黄)"<<Possiblity("伟","黄")<<endl;
        outlog<<"finish"<<endl;
    }

};


}

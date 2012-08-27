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
    WordLevelDBTable dbTable_Ngram;
    int HMMmodleSize;
    fstream outlog;
    vector<string>  example;
    string scdPath;
public:
     
   Ngram(string collectionpath)
   {
     string leveldbpath=collectionpath+"/Ngram";
     dbTable_Ngram.open(leveldbpath);
     string path=collectionpath+"/Ngram.log";
     scdPath=collectionpath+"/B-00-201207171255-46229-I-C.SCD";
     outlog.open(path.c_str(), ios::out);
     HMMmodleSize=3;
     outlog<<"start"<<endl;
     
    };
   ~Ngram()
   {
     dbTable_Ngram.close();
     outlog.close();
   };
  void LoadFile()
  {
         string filePath=scdPath;
  
        ifstream in;
        //out<<*it<<endl;
        string filename = filePath;
        in.open(filename.c_str(), ios::in);
        //out<<"open scd"<<endl;
            int IntegrationNumber=0;
            int numberMax=100;
            vector<string> exampleTemp; 
            vector<string> result;
        if (in.good())
        {
            //out<<"in good"<<endl;
            int size;
            std::string temp;
            string Title="<Title>";
            string Content="<Content>";
            string ProdName="<ProdName>";
            string Integration="<Integration>";
            uint32_t count = 0;
             string dot="，";
            
            while(count <100000)
            {   outlog<<count<<endl;
                getline(in, temp);
                if(temp.substr(0,Title.size())==Title)
                {   temp=temp.substr(Title.size());
                    int len=0;
                    int templen; 
                    while(len<temp.length())
                    {   int len1=temp.find("，");
                        int len2=temp.find("。");
                        if(len1!=string::npos&&len2!=string::npos)
                        {
                           if(len1==string::npos)
                           {
                             templen=len2;
                             
                           }
                           else{
                           if(len2==string::npos)
                           {
                             templen=len1;
                            
                           }
                           else
                           {
                             templen=min(len1,len2);
                             
                           }
                           }
                           computeString( temp.substr(templen));
                           temp=temp.substr(templen+dot.length(),temp.size()-templen+dot.length());
                        }
                        else
                        {templen=temp.length();
                         computeString( temp.substr(templen));
                         break;
                        }
                        
                    }
                }
                else
                {
                
                if(temp.substr(0,Content.size())==Content)
                {
                    temp=temp.substr(Content.size());
                    int len=0;
                    int templen; 
                    while(len<temp.length())
                    {    int len1=temp.find("，");
                        int len2=temp.find("。");
                        if(len1!=string::npos&&len2!=string::npos)
                        {
                           if(len1==string::npos)
                           {
                             templen=len2;
                             
                           }
                           else{
                           if(len2==string::npos)
                           {
                             templen=len1;
                            
                           }
                           else
                           {
                             templen=min(len1,len2);
                             
                           }
                           }
                           computeString( temp.substr(templen));
                           temp=temp.substr(templen+dot.length(),temp.size()-templen+dot.length());
                        }
                        else
                        {templen=temp.length();
                         computeString( temp.substr(templen));
                         break;
                        }
                    }
                }
                else
                {
                if(temp.substr(0,ProdName.size())==ProdName)
                {   
                    
                    temp=temp.substr(ProdName.size());
                    int len=0;
                    int templen; 
                    while(len<temp.length())
                    {   int len1=temp.find("，");
                        int len2=temp.find("。");
                        if(len1!=string::npos&&len2!=string::npos)
                        {
                           if(len1==string::npos)
                           {
                             templen=len2;
                             
                           }
                           else{
                           if(len2==string::npos)
                           {
                             templen=len1;
                            
                           }
                           else
                           {
                             templen=min(len1,len2);
                             
                           }
                           }
                           computeString( temp.substr(templen));
                           temp=temp.substr(templen+dot.length(),temp.size()-templen+dot.length());
                        }
                        else
                        {templen=temp.length();
                         computeString( temp.substr(templen));
                         break;
                        }
                    }
                }
                else
                {
                if(temp.substr(0,Integration.size())==Integration)
                {   
                    temp=temp.substr(Integration.size());
                    int len=0;
                    int templen; 
                    IntegrationNumber++;
                    if( IntegrationNumber<100)
                    {
                        result.push_back(temp);
                    }
                    while(len<temp.length())
                    {    int len1=temp.find("，");
                        int len2=temp.find("。");
                        if(len1!=string::npos&&len2!=string::npos)
                        {
                           if(len1==string::npos)
                           {
                             templen=len2;
                             
                           }
                           else{
                           if(len2==string::npos)
                           {
                             templen=len1;
                            
                           }
                           else
                           {
                             templen=min(len1,len2);
                             
                           }
                           }
                           computeString( temp.substr(templen));
                           temp=temp.substr(templen+dot.length(),temp.size()-templen+dot.length());
                        }
                        else
                        {templen=temp.length();
                         computeString( temp.substr(templen));
                         break;
                        }
                    }
                }
                }
                }
                }
                count++;
            }
            in.close();

        }
        else
        {
            //out<<"in bad"<<endl;
            in.close();
        }
        for(int i=0;i<result.size();i++)
        { outlog<<result[i]<<endl;
        }
        example=result;
  }
   vector<string> getExample()
   {
        return example;
   }
  void computeString(string str)
  {   

    izenelib::util::UString ustr(str, izenelib::util::UString::UTF_8);
    // outlog<<"computeString1"<<endl;
     string HitNumStr;
     string strseg;
     int HitNum;
     for(int i=0;i<ustr.length();i++)
     {   
        //outlog<<"computeString2"<<endl;
        for(int k=max(0,i-HMMmodleSize+1);k<=i;k++)
        {
        //outlog<<"i"<<i<<"      k"<<k<<endl;
        izenelib::util::UString ustrseg=ustr.substr(k,i-k+1);
        ustrseg.convertString(strseg, UString::UTF_8);
        //outlog<<"strseg         "<<strseg<<endl;
        if(dbTable_Ngram.get_item(strseg, HitNumStr))
        {
            if(HitNumStr.length()!=0)
            {
                 HitNum=boost::lexical_cast<int>(HitNumStr)+1;
                 HitNumStr=boost::lexical_cast<string>(HitNum);
            }
            else
            {
                HitNumStr=boost::lexical_cast<string>(1);
            }
        }
        else
        {
            HitNumStr=boost::lexical_cast<string>(1);
        }
        dbTable_Ngram.add_item(strseg, HitNumStr);
        }
        //outlog<<"computeString3"<<endl;
     }
  }
  

  double Pos(string str)
  {     
        string HitNumStr;
        if(dbTable_Ngram.get_item(str, HitNumStr))
        {    //outlog<<"Possiblity2"<<endl;
            if(HitNumStr.length()!=0)
            {
               int p=boost::lexical_cast<int>(HitNumStr);
               return double(p);
            }
        } 
        
        //outlog<<"Possiblity3"<<endl; 
        return 0;
  }
  double Pos(string str ,string str1)
  {     //outlog<<"Possiblity1"<<endl; 
        string HitNumStr;
        dbTable_Ngram.get_item(str1+str, HitNumStr);
       
        if(dbTable_Ngram.get_item(str1+str, HitNumStr))
        {  //  outlog<<"Possiblity2"<<endl;
            if(HitNumStr.length()!=0)
            {
               int p=boost::lexical_cast<int>(HitNumStr);
               return double(p);
            }
        } 
        
        //outlog<<"Possiblity3"<<endl; 
        return 0;
  }
  double Pos(string str ,string str1,string str2)
  {     string HitNumStr;
        dbTable_Ngram.get_item(str2+str1+str, HitNumStr);
       
        if(dbTable_Ngram.get_item(str2+str1+str, HitNumStr))
        {  //  outlog<<"Possiblity2"<<endl;
            if(HitNumStr.length()!=0)
            {
               int p=boost::lexical_cast<int>(HitNumStr);
               return double(p);
            }
        } 
        
        //outlog<<"Possiblity3"<<endl; 
        return 0;
  }


  double Possiblity(string str)
  {
       return Pos(str);
  }
  double Possiblity(string str ,string str1)
  {   
      if(Pos(str1)!=0)
      {
       return Pos(str,str1)/Pos(str1);
      }
  }
  double Possiblity(string str ,string str1,string str2)
  {
      if(Pos(str1,str2)!=0)
      {
       return Pos(str,str1,str2)/Pos(str1,str2);
      }
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

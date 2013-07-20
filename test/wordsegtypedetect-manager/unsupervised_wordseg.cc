

#include "latticelmMap.h"
#include <iostream>
#include <fstream>
#include <string>   
#include <boost/algorithm/string.hpp>
#include <set>
#include <map>
#include <boost/tuple/tuple.hpp>
#include <idmlib/util/directory_switcher.h>
using namespace std;
string categoryNow;
set<string> categorySet;
map<string,string> categoryFileMap;

map<string,boost::tuple<double,int,string> > stringNumMap;
map<string,bool> brandvec;
struct node
{
    string str;
    map<int,double> weightMap;
};
bool filter(const string& brand)
{
     izenelib::util::UString ubrand(brand,izenelib::util::UString::UTF_8);
     if(brand.size()>18)
            return false;
     if(brand.find("：")!=string::npos)
            return false;
     if(brand.find("�")!=string::npos)
            return false;
     if(brand.find("）")==0)
            return false;
     if(brand.find("！")==0)
            return false;
     if(brand.find("男")==0)
            return false;
     if(brand.find("女")==0)
            return false;
     if(brand.find("版")==0)
            return false;
     if(brand.find("灯")==0)
            return false;
     if(brand.find("潮")==0)
            return false;
     if(brand.find("每")==0)
            return false;
     if(brand.find("款")==0)
            return false;
     if(brand.find("送")==0)
            return false;
     if(brand.find("裤")==0)
            return false;
     if(brand.find("衫")==0)
            return false;
     if(brand.find("正品")!=string::npos)
            return false;
     if(brand.find("包邮")!=string::npos)
            return false;
     if(brand.find("式")==0)
            return false;
     if(brand.find("年")==0)
            return false;
     if(brand.find("寸")==0)
            return false;
     if(brand.find("型")==0)
            return false;
     if(brand.find("味")==0)
            return false;
     if(brand.find("号")==0)
            return false;
     if(brand.find("包")==0)
            return false;
     if(brand.find("克")==0)
            return false;
     if(brand.find("+")==0)
            return false;
     if(brand.find("*")==0)
            return false;
     if(brand.find("-")==0)
            return false;
     if(brand.find("】")==0)
            return false;
     if(brand.find("&")==0)
            return false;
     if(brand.find("<")==0)
            return false;
     if(brand.find(":")==0)
            return false;
     if(brand.find("#")==0)
            return false;
     if(brand.find(",")==0)
            return false;
     if(brand.find("、")==0)
            return false;
     if(brand.find("『")==0)
            return false;
     if(brand.find("・")==0)
            return false;
     if(brand.find("㊣")==0)
            return false;
     if(brand.find("★")==0)
            return false;
     if(brand.find("』")==0)
            return false;
     if(brand.find("!")==0)
            return false;
     if(brand.find("牌")==0)
            return false;
     if(brand.find("片")==0)
            return false;
     if(brand.find("及")==0)
            return false;
     if(brand.find("和")==0)
            return false;
     if(brand.find("度")==0)
            return false;
     for(size_t i=0;i<ubrand.size();i++)
     {
            if(ubrand.isNumericChar(i))
            {
                 return false;
            }
     }
     return true;
}
bool rewirte(string& line)
{
    //boost::algorithm::replace_all(line,"/t","");
    string attr=line.substr(0,line.find(">")+1);
    line=line.substr(line.find(">")+1);


    {
        if(attr=="<DOCID>")
        {
              categoryNow="";
              return false;
        }
        if(attr=="<Category>")
        {
              categoryNow=line;
              categorySet.insert(line);
              return false;
        }
        if(attr=="<Title>")
              return true;
        else
              return false;
    }
};
vector<string> getSegment(string temp,string symbol=" ")
{  
    string line=temp;
    size_t templen = temp.find(symbol);
    vector<string> ret;
    while(templen!= string::npos)
    {
        if(templen!=0)
        {
            ret.push_back(temp.substr(0,templen));
        }
        temp=temp.substr(templen+1);
        templen = temp.find(symbol);

    }
    if(temp.length()>0)
    {
        ret.push_back(temp);
    }
    return ret;
}
int main(int argc, char** argv)
{  
    LatticeLMMap lmmap(argv[1],argv[2]);
    lmmap.load();
    string ret;
    lmmap.segWord("恒亮破壁花粉230.1克美丽容颜皮肤可直接吸收因此可作面膜","传统",ret);
    //恒 亮 破 壁 花 粉 230.1 克 美 丽 容 颜 皮 肤 可 直 接 吸 收 因 此 可 作 面 膜",
    cout<<ret<<endl;
    ifstream in;
    in.open(argv[3],ios::in);
    ofstream out;
    ifstream bra;
    ofstream seg;
    seg.open("segresult",ios::out);
    out.open("stringnum",ios::out);
    bra.open("brand.txt",ios::in);
    string line;
    boost::posix_time::time_duration gtime;
    if(bra.is_open())
    {
         while(!bra.eof())
         {
              getline(bra,line);
              brandvec.insert(make_pair(line,true));
         }
    }
    bra.close();
    if(in.is_open())
    {
         int i=0;
         while(!in.eof())
         {

              getline(in,line);
              if(!rewirte(line))
                  continue;
              if(line.length()>2&&line!="\n"&&categoryNow.length()>0)
              {
                  boost::posix_time::ptime time_now = boost::posix_time::microsec_clock::local_time();
                  string ret;
                  if(categoryNow.find("书籍")==string::npos)
                  {
                     if(lmmap.segWord(line,categoryNow,ret))
                     {
                         vector<string> vec=getSegment(ret);
                         for(int j=0;j<vec.size();j++)
                              if(vec[j].size()>3)
                              {
                                  if(stringNumMap.find(vec[j])==stringNumMap.end())
                                  {
                                        stringNumMap[vec[j]]=boost::make_tuple(j,1,categoryNow);
                                  }
                                  else
                                  {
                                        stringNumMap[vec[j]].get<0>()+=j;//double(j)/vec.size();
                                        stringNumMap[vec[j]].get<1>()++;
                                  }
                              }
                         seg<<ret<<endl;
                         i++;
                      }
                  }
              }
              if(i%1000==0)
                  cout<<"time"<<boost::posix_time::to_iso_string(gtime)<<endl;
         }
         
    }
    int rightNum=0;
    int allnum=0;
    string lastbrand="dasdasds";
    int lasthit=0;
    string buff;
    for(map<string,boost::tuple<double,int,string> >::iterator iter=stringNumMap.begin();iter!=stringNumMap.end();iter++)
    { 
          if((iter->second).get<0>()/(iter->second).get<1>()<1.3&&(iter->second).get<1>()>20&&filter(iter->first))//
          {
              boost::algorithm::replace_all(buff,"(","");
              boost::algorithm::replace_all(buff,"（","");
              boost::algorithm::replace_all(buff,"）","");
              boost::algorithm::replace_all(buff,")","");
              boost::algorithm::replace_all(buff,"/","");
              boost::algorithm::replace_all(buff,"\\","");
              boost::algorithm::replace_all(buff,"【","");
              boost::algorithm::replace_all(buff,"】","");             
              boost::algorithm::replace_all(buff,"★","");
              boost::algorithm::replace_all(buff,"◎","");
              boost::algorithm::replace_all(buff,"■","");
              boost::algorithm::replace_all(buff,"◆","");
              boost::algorithm::replace_all(buff,"★","");
              boost::algorithm::replace_all(buff,"★","");
              boost::algorithm::replace_all(buff,"◢","");
              boost::algorithm::replace_all(buff,"◤","");
              boost::algorithm::replace_all(buff,"、","");
              boost::algorithm::replace_all(buff,"，","");
              boost::algorithm::replace_all(buff,"\"","");
              boost::algorithm::replace_all(buff,";","");
              boost::algorithm::replace_all(buff,"‖","");
              boost::algorithm::replace_all(buff,"\"","");
              boost::algorithm::replace_all(buff,"”","");
              if((iter->first).find(lastbrand)==string::npos)
              {
                  boost::algorithm::replace_all(buff,"(","");
                  boost::algorithm::replace_all(buff,")","");
                  boost::algorithm::replace_all(buff,"/","");
                  boost::algorithm::replace_all(buff,"\\","");
                  boost::algorithm::replace_all(buff,"【","");
                  boost::algorithm::replace_all(buff,"】","");
                  if(buff.size()>3)
                      out<<buff.c_str()<<endl;
                  buff=iter->first;//+"\t"+(iter->second).get<2>();
              }
              else if(lasthit<(iter->second).get<1>())
              {
                  boost::algorithm::replace_all(buff,"(","");
                  boost::algorithm::replace_all(buff,")","");
                  boost::algorithm::replace_all(buff,"/","");
                  boost::algorithm::replace_all(buff,"\\","");
                  boost::algorithm::replace_all(buff,"【","");
                  boost::algorithm::replace_all(buff,"】","");
                  buff=iter->first;//+"\t"+(iter->second).get<2>();
              }
              else
              {
                 continue;
              }
              lastbrand=iter->first;
              lasthit=(iter->second).get<1>();
              allnum++;   
/**/           
          }

    }
    cout<<"accurancy"<<double(rightNum)/allnum<<endl;
    cout<<"recall"<<double(rightNum)/brandvec.size()<<endl;
    in.close();
    out.close();
    seg.close();
}


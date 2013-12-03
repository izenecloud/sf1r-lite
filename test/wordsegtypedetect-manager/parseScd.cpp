#include<iostream>
#include<fstream>
#include <string>   
#include <boost/algorithm/string.hpp>
#include <set>
#include <map>
#include <vector>
#include <boost/algorithm/string/trim.hpp>
using namespace std;
map<string,string> attr;
string title;
ofstream out;
vector<string> brand;
string categoryNow="";
string categoryThis;
vector<string> getSegment(string temp,string symbol=",")
{   string line=temp;
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
void output()
{
    vector<pair<int,string> > vec;
    for(map<string,string>::iterator iter=attr.begin();iter!=attr.end();iter++)
    {
        if(title.find(iter->first)!=string::npos)
        {
            vec.push_back(pair<int,string>(title.find(iter->first),iter->first));
        }
    }
    sort(vec.begin(),vec.end());
    string buff;
    bool print=false;
    for(size_t i=0;i<vec.size();i++)
    {
        string seco=attr[vec[i].second];
        boost::algorithm::trim(vec[i].second);
        boost::algorithm::replace_all(vec[i].second," ","");
        boost::algorithm::replace_all(vec[i].second,"\t","");
        boost::algorithm::trim(seco);
        boost::algorithm::replace_all(seco," ","");
        boost::algorithm::replace_all(seco,"\t","");
        if(seco=="品牌")
           buff=vec[i].second;
           // brand.push_back(vec[i].second);
        if(seco=="型号")
        {
           buff+=" "+vec[i].second;//" "<<seco<<endl;
           print=true;
        }
    }
    if(print)
        out<<buff<<endl;
    //out<<endl;

}
void parseAttr(string& line)
{
    vector<string> attrpair=getSegment(line);
    for(size_t i=0;i<attrpair.size();i++)
    {
       vector<string> att=getSegment(attrpair[i],":");
       for(size_t i=0;i<att.size();i++)
           cout<<att[i]<<endl;
       if(att.size()==2)
       {
          if(att[0].find("_optional")!=string::npos)
          {
             att[0]=att[0].substr(0,att[0].length()-9);
          }
          attr[att[1]]=att[0];
       }

    }
}
bool exec(string& line)
{
    //boost::algorithm::replace_all(line,"/t","");
    string atta=line.substr(0,line.find(">")+1);
    line=line.substr(line.find(">")+1);
    if(atta=="<DOCID>")
    {
        output();
        attr.clear();

    }
    if(atta=="<Attribute>")
    {
       if(categoryNow.find(categoryThis)!=string::npos)
        parseAttr(line);
    }
    if(atta=="<Category>")
    {
        categoryNow=line;
    }
    if(atta=="<Title>")
    {
        title=line;
    }
    return true;
};
int main(int argc, char** argv)
{  
    ifstream in;
    in.open(argv[1],ios::in);
    categoryThis=string(argv[2]);
    out.open((categoryThis+".brand").c_str(),ios::out);
    string line;
    if(in.is_open())
    {
        // size_t i=0;
         while(!in.eof())
         {
              getline(in,line);
              exec(line);
         }
         
    }
    sort(brand.begin(),brand.end());
    brand.erase(unique(brand.begin(),brand.end()),brand.end());
    for(size_t i=0;i<brand.size();i++)
        out<<brand[i]<<endl;
    in.close();
    out.close();
}

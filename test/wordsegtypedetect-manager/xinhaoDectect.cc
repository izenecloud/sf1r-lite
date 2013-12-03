#include <boost/filesystem.hpp> 
#include <util/ustring/UString.h>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
using namespace std;
enum chartype {NUM=0,CHN,EN,SYMBOL};
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
vector<pair<string,chartype> > segString(std::string &sentence)
{

     size_t size=0;
     izenelib::util::UString usentence(sentence,izenelib::util::UString::UTF_8);
     izenelib::util::UString buff;
     vector<izenelib::util::UString> ret;
     vector<chartype> typeret;
     vector<pair<string,chartype> > sret;

     chartype origin=NUM;
     chartype now;
     for(size_t i=0;i<usentence.size();i++)
     {
            now=SYMBOL;
            if(usentence.isChineseChar(i))
            {
                 now=CHN;
            }
            if(usentence.isNumericChar(i))
            {
                 now=NUM;
            }
            if(usentence.isLowerChar(i))
            {
                 now=EN;
            }

            if(origin==now) 
                 buff.append(usentence.substr(i,1));
            else
            {
                 if(i!=0)
                 {
                    ret.push_back(buff);
                    typeret.push_back(origin);
                 }
                 buff=usentence.substr(i,1);
                 origin=now;
            }
      }
      ret.push_back(buff);
      typeret.push_back(now);
      for(int i=0;i<ret.size();i++)
      {
            string temp;
            ret[i].convertString(temp, izenelib::util::UString::UTF_8);
            if(temp.size()>0)
                sret.push_back(make_pair(temp,typeret[i]));
      }
      return sret;
}
string getTag(pair<string,chartype> p)
{
     if(p.second==CHN)
            return "中文";
     if(p.second==EN)
            return "英文";
     if(p.second==NUM)
            return boost::lexical_cast<string>(p.first.size())+"位数字";
     return "符号";
}
void processNum()
{
}
int main(int argc, char** argv)
{  
    ifstream in;
    in.open( (string(argv[1])+".brand").c_str(),ios::in);
    ofstream out;
    out.open( (string(argv[1])+".train").c_str(),ios::out);
    string line;
    if(in.is_open())
    {
        while(!in.eof())
        {
           getline(in,line);
           boost::to_lower(line);
           vector<string> a=getSegment(line);
           if(a.size()!=2)
                continue;
           else
           {
                vector<pair<string,chartype> > segR=segString(a[1]);
                for(int i=0;i<segR.size();i++)
                {
                     out<<segR[i].first<<" "<<getTag(segR[i])<<" "<<a[0]<<endl;
                }
                out<<endl;
           }
        }
    }
    out.close();
    in.close();
}
    


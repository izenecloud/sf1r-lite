#ifndef SF1R_AD_BIAS_HPP_
#define SF1R_AD_BIAS_HPP_
#include <log-manager/LogAnalysis.h>
#include <string.h>
#include <iostream>
#include <list>
#include <map>
#include <vector>
#include <time.h>
#include <log-manager/UserQuery.h>
using namespace std;
using namespace boost;

namespace sf1r
{

class AdBias
{
    typedef map<string,uint32_t> StrToIntMap;
    StrToIntMap adverWeight;
public:
    AdBias(string path)
    {
        cout<<"AdBiasBuild"<<endl;
        initFromDB();
        initFromTxt(path);
    };
    ~AdBias()
    {

    };
    void initFromDB()
    {
        string collectionName_="b5mp";
        boost::posix_time::ptime time_now = boost::posix_time::microsec_clock::local_time();
        boost::posix_time::ptime p = time_now - boost::gregorian::days(120);
        std::string time_string = boost::posix_time::to_iso_string(p);
        std::vector<UserQuery> query_records;

        using namespace std;
        using namespace boost;
        UserQuery::find(
            "query ,max(hit_docs_num) as hit_docs_num, count(*) as count ",
            "collection = '" + collectionName_ + "' and hit_docs_num > 0 AND TimeStamp >= '" + time_string + "'",
            "query",
            "",
            "",
            query_records);
        std::vector<UserQuery>::const_iterator it = query_records.begin();
        for (; it != query_records.end(); ++it)
        {
            adverWeight.insert(pair<string,uint32_t>(it->getQuery(),it->getCount()));
        }

    };
    void initFromTxt(string path)
    {
          ifstream in;
          in.open(path.c_str(),ios::in);
          while(!in.eof())
          {
                 string word;
                 getline(in,word);
                 adverWeight.insert(pair<string,uint32_t>(word,20));
          }

    };
    void initFromScd()
    {

    };
    uint32_t getCount(string keyword)
    {
        std::map<string,uint32_t>::iterator it= adverWeight.find(keyword);
        if(it == adverWeight.end())
        {
            return 0;
        }
        else
        {
            return it->second;
        }

    };
};

}
#endif

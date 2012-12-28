#ifndef SF1R_AD_BIAS_HPP_
#define SF1R_AD_BIAS_HPP_

#include <iostream>
#include <list>
#include <map>
#include <vector>
#include <log-manager/UserQuery.h>
#include <log-manager/LogAnalysis.h>

namespace sf1r
{

class AdBias
{
    typedef std::map<std::string,uint32_t> StrToIntMap;
    StrToIntMap adverWeight;
public:
    AdBias(const std::string& path)
    {
        std::cout<<"AdBiasBuild"<<std::endl;
        //InitFromDB_();
        InitFromTxt_(path);
    }

    ~AdBias()
    {
    }

    void InitFromDB_()
    {
        std::string collectionName_="b5mp";
        boost::posix_time::ptime time_now = boost::posix_time::microsec_clock::local_time();
        boost::posix_time::ptime p = time_now - boost::gregorian::days(120);
        std::string time_string = boost::posix_time::to_iso_string(p);
        std::vector<UserQuery> query_records;

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
            adverWeight.insert(std::pair<std::string,uint32_t>(it->getQuery(),it->getCount()));
        }

    }

    void InitFromTxt_(const std::string& path)
    {
        std::ifstream in;
        in.open(path.c_str(),std::ios::in);
        while(!in.eof())
        {
            std::string word;
            getline(in,word);
            adverWeight.insert(std::pair<std::string,uint32_t>(word,20));
        }
    }

    void InitFromScd_()
    {
    }

    uint32_t GetCount(const std::string& keyword)
    {
        std::map<std::string,uint32_t>::const_iterator it= adverWeight.find(keyword);
        if(it == adverWeight.end())
        {
            return 0;
        }
        else
        {
            return it->second;
        }
    }
};

}
#endif

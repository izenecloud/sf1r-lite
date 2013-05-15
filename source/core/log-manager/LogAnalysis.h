#ifndef LOGANALYSIS_H_
#define LOGANALYSIS_H_

#include <string>
#include <time.h>
#include <fstream>
#include <iostream>
#include "UserQuery.h"

#include <util/ustring/UString.h>
#include <util/ustring/algo.hpp>
using namespace std;
namespace sf1r
{
typedef uint32_t FREQ_TYPE;
struct QueryType
{
        std::string strQuery_;
        FREQ_TYPE freq_;
        uint32_t HitNum_;
        inline bool operator > (const QueryType other) const
        {
            return freq_< other.freq_ ;
        }

        inline bool operator < (const QueryType other) const
        {
            return freq_ > other.freq_;
        }
        inline bool operator == (const QueryType other) const
        {
            return freq_ == other.freq_;
        }
        inline bool cmp(const QueryType other) const
        {
                    string strA=strQuery_;
                    string strB =other.strQuery_;
                    boost::algorithm::replace_all(strA, " ", "");
                    boost::algorithm::replace_all(strB, " ", "");
                    std::transform(strA.begin(), strA.end(), strA.begin(), ::tolower);
                    std::transform(strB.begin(), strB.end(), strB.begin(), ::tolower);
                    return strA == strB;
        }


};
struct QueryTypeToDeleteDup
{
      std::string strQuery_;
      QueryType qt_;
      inline bool operator > (const QueryTypeToDeleteDup other) const
      {
            return strQuery_< other.strQuery_ ;
      }

      inline bool operator < (const QueryTypeToDeleteDup other) const
      {
            return strQuery_ > other.strQuery_;
      }
      inline bool operator == (const QueryTypeToDeleteDup other) const
      {
            return strQuery_ == other.strQuery_;
      }
      void initfrom(QueryType qt)
      {
            qt_=qt;
            string strA=qt.strQuery_;

            boost::algorithm::replace_all(strA, " ", "");

            std::transform(strA.begin(), strA.end(), strA.begin(), ::tolower);
            strQuery_=strA;
      }
      QueryType getQueryType()
      {
            return qt_;
      }
};
struct queryover
{
    bool operator() (const QueryType Q1,const QueryType Q2)
    {
             return Q1.strQuery_ > Q2.strQuery_;
    }

 } ;//queryover;
 struct queryequal
 {
    bool operator() (const QueryType Q1,const QueryType Q2)
    {
              string strA=Q1.strQuery_;
              string strB =Q2.strQuery_;
              boost::algorithm::replace_all(strA, " ", "");
              boost::algorithm::replace_all(strB, " ", "");
              std::transform(strA.begin(), strA.end(), strA.begin(), ::tolower);
              std::transform(strB.begin(), strB.end(), strB.begin(), ::tolower);
              return strA == strB;;
    }

 } ;//queryequal;
class LogAnalysis
{

public:

    static void getRecentKeywordList(const std::string& collectionName, uint32_t limit, std::vector<izenelib::util::UString>& recentKeywords);
    static void getRecentKeywordFreqList(const std::string& collectionName, const std::string& time_string, std::vector<UserQuery>& queryList);
    static void getRecentKeywordFreqList(const std::string& time_string, std::vector<UserQuery>& queryList);


private:

    LogAnalysis() {}

};

}

#endif

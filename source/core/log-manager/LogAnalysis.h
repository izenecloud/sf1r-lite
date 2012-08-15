#ifndef LOGANALYSIS_H_
#define LOGANALYSIS_H_

#include <string>
#include <time.h>
#include <fstream>
#include <iostream>

#include <util/ustring/UString.h>
#include <util/ustring/algo.hpp>

namespace sf1r
{
typedef uint32_t FREQ_TYPE;
/*  struct QueryType
  {
std::string strQuery_;
FREQ_TYPE freq_;
  };
*/
struct QueryType
{
    std::string strQuery_;
    FREQ_TYPE freq_;
    uint32_t HitNum_;
};
class LogAnalysis
{

public:

    static void getRecentKeywordList(const std::string& collectionName, uint32_t limit, std::vector<izenelib::util::UString>& recentKeywords);
    static void getRecentKeywordFreqList(const std::string& collectionName, uint32_t limit, std::vector<QueryType>& queryList);


private:

    LogAnalysis() {}

};

}

#endif

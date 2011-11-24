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

class LogAnalysis
{

public:

    static void getRecentKeywordList(const std::string& collectionName, uint32_t limit, std::vector<izenelib::util::UString>& recentKeywords);


private:

    LogAnalysis() {}

};

}

#endif

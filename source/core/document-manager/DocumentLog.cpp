#include <time.h>

#include <iostream>
#include <string>

#include "DocumentLog.h"

namespace sf1r
{

using namespace std;

const string DocumentLog::logTypeStrings_[6] =
{
    "info", "debug", "warn", "error", "crit", "perf"
};


bool DocumentLog::print(LogType             type,
                        const std::string&  module,
                        const std::string&  message,
                        std::ostream&       os)
{
#ifndef SF1_DEBUG
    if (type == TYPE_INFO)
        return true;
#endif
#ifndef SF1_TIME_CHECK
    if (type == TYPE_PERF)
        return true;
#endif
    time_t t = time(NULL);
    struct tm* local = localtime(&t);
    os  << "[" << (1900+ local->tm_year) << "-" << (1 + local->tm_mon) << "-" << local->tm_mday \
    << " " << local->tm_hour << ":" << local->tm_min << ":" << local->tm_sec \
    << "][DocumentManager][" << module << "][" << logTypeStrings_[type] \
    << "][" << message << "]" << endl;
    return true;
}

} // end - namespace sf1r*/

/*
int main()
{
    DocumentLog::print(DocumentLog::TYPE_INFO, "main", "test");
    return 0;
}
*/

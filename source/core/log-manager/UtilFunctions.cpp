#include "UtilFunctions.h"

#include <time.h>

namespace sf1r {

time_t createTimeStamp()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

time_t createTimeStamp(boost::posix_time::ptime pt)
{
    boost::posix_time::ptime epoch(boost::gregorian::date(1970, 1, 1));
    return (pt - epoch).total_microseconds() + timezone * 1000000;
}

time_t createTimeStamp(const izenelib::util::UString& text)
{
    std::string str;
    text.convertString(str, izenelib::util::UString::UTF_8);
    return createTimeStamp(str);
}

// Return -1 when empty, -2 when invalid.
time_t createTimeStamp(const string& text)
{
    if (text.empty()) return -1;
    boost::gregorian::date date;
    try
    {
        date = boost::gregorian::from_string(text);
    }
    catch (const std::exception& ex)
    {
    }
    if (!date.is_not_a_date())
    {
        return createTimeStamp(boost::posix_time::ptime(date));
    }
    if (text.length() < 8) return -2;
    std::string cand_text = text.substr(0, 8);
    try
    {
        date = boost::gregorian::from_undelimited_string(cand_text);
    }
    catch (const std::exception& ex)
    {
    }
    if (!date.is_not_a_date())
    {
        return createTimeStamp(boost::posix_time::ptime(date));
    }
    return -2;
}

}

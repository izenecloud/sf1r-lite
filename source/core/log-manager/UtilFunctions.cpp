#include "UtilFunctions.h"

namespace sf1r {

time_t to_time_t(boost::posix_time::ptime pt)
{
    boost::posix_time::ptime epoch(boost::gregorian::date(1970, 1, 1));
    boost::posix_time::time_duration::sec_type x = (pt - epoch).total_seconds();

    return time_t(x);
}

bool timeFromUString(time_t& tt, const izenelib::util::UString& text)
{
    std::string str;
    text.convertString(str, izenelib::util::UString::UTF_8);
    return timeFromString(tt, str);
}

bool timeFromString(time_t& tt, const string& text)
{
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
        tt = to_time_t(boost::posix_time::ptime(date));
        return true;
    }
    if (text.length() < 8) return false;
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
        tt = to_time_t(boost::posix_time::ptime(date));
        return true;
    }
    return false;
}

}

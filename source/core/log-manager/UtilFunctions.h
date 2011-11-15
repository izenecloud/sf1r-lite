#ifndef _LOG_MANAGER_UTIL_FUNCTIONS_H_
#define _LOG_MANAGER_UTIL_FUNCTIONS_H_

#include <util/ustring/UString.h>

#include <string>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace sf1r {

template <typename T>
std::string toBytes(const T& val)
{
    return std::string(reinterpret_cast<const char *>(&val), sizeof(T));
}

template <typename T>
T fromBytes(const std::string& str)
{
    return *(reinterpret_cast<const T *>(str.c_str()));
}

time_t to_time_t(boost::posix_time::ptime pt);

bool timeFromUString(time_t& tt, const izenelib::util::UString& text);

bool timeFromString(time_t& tt, const std::string& text);

}

#endif

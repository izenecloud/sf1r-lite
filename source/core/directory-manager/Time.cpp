/**
 * @file core/directory-manager/Time.cpp
 * @author Ian Yang
 * @date Created <2009-10-27 16:59:10>
 * @date Updated <2009-10-30 15:43:16>
 * @brief
 */
#include "Time.h"

#include <boost/date_time/posix_time/posix_time_types.hpp>

namespace sf1r {
namespace directory {

using boost::gregorian::date;
using boost::posix_time::ptime;
using boost::posix_time::second_clock;
using boost::posix_time::time_duration;

long Time::secondsSinceEpoch()
{
    const ptime kEpoch(date(1970, 1, 1));

    ptime now(second_clock::universal_time());
    time_duration diff = now - kEpoch;
    return diff.total_seconds();
}

}} // namespace sf1r::directory

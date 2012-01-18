#include "Utilities.h"

#include <ir/index_manager/utility/StringUtils.h>
#include <util/mkgmtime.h>

#include <boost/token_iterator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <vector>

using namespace boost::gregorian;
using namespace boost::posix_time;
using namespace izenelib::ir::indexmanager;

namespace sf1r
{

void gregorianISO8601Parser(const std::string& dataStr, struct tm& atm)
{
    try
    {
        if(dataStr.find(" ") != std::string::npos)
        {
            ptime t(time_from_string(dataStr));
            atm = to_tm(t);
        }
        else
        {
            date d(from_simple_string(dataStr));
            atm = to_tm(d);
        }
    }
    catch (const std::exception& e)
    {
        ptime now = second_clock::local_time();
        atm = to_tm(now);

        if (dataStr.find("/") != std::string::npos)
        {
            try
            {
                std::vector<std::string> ps = split(dataStr, "/");
                size_t len = ps.size();
                if (len == 3)
                {
                    size_t year = boost::lexical_cast<size_t>(ps[len - 1]);
                    size_t month = boost::lexical_cast<size_t>(ps[len - 2]);
                    size_t day = boost::lexical_cast<size_t>(ps[len - 3]);
                    if (year > 1900 && month <= 12 && month > 0 && day <= 31 && day > 0)
                    {
                        /// format 10/02/2009
                        atm.tm_year = year > 1900 ? (year - 1900) : (year + 2000 - 1900);   // tm_year is 1900 based
                        atm.tm_mon = month >= 0 ? month : 0;                                // tm_mon is 0 based
                        atm.tm_mday = day >0 ? day : 1;
                    }
                }
            }
            catch (const std::exception& e)
            {
                ptime now = second_clock::local_time();
                atm = to_tm(now);
            }
        }
    }
}

void gregorianISOParser(const std::string& dataStr, struct tm& atm)
{
    try
    {
        date d(from_undelimited_string(dataStr));
        date::ymd_type ymd = d.year_month_day();
        size_t year = ymd.year;
        size_t month = ymd.month;
        size_t day = ymd.day;

        atm.tm_year = year > 1900 ? (year - 1900) : (year + 2000 - 1900);   // tm_year is 1900 based
        atm.tm_mon = month >= 0 ? month : 0;                                // tm_mon is 0 based
        atm.tm_mday = day > 0 ? day : 1;

    }
    catch(std::exception& e)
    {
        ptime now = second_clock::local_time();
        atm = to_tm(now);
    }
}

bool isAllDigit(const string& str)
{
    size_t i ;
    for (i = 0; i != str.length(); i++)
    {
        if (!isdigit(str[i]))
        {
            return false;
        }
    }
    return true;
}

void convert(const std::string& dataStr, struct tm& atm)
{
    if (dataStr.find_first_of("/-") != std::string::npos)
    {
        /// format 2009-10-02
        /// format 2009-10-02 23:12:21
        /// format 2009/10/02
        /// format 2009/10/02 23:12:21
        /// format 10/02/2009
        gregorianISO8601Parser(dataStr, atm);
    }
    else if (dataStr.size() == 14 && isAllDigit(dataStr))
    {
        /// format 20091009163011
        try
        {
            int offsets[] = {4, 2, 2, 2, 2, 2};

            boost::offset_separator f(offsets, offsets+5);
            typedef boost::token_iterator_generator<boost::offset_separator>::type Iter;
            Iter beg = boost::make_token_iterator<string>(dataStr.begin(), dataStr.end(), f);
            Iter end = boost::make_token_iterator<string>(dataStr.end(), dataStr.end(), f);
            int datetime[6];
            memset(datetime, 0, 6 * sizeof(int));
            for (int i = 0; beg != end; ++beg, ++i)
            {
                datetime[i] = boost::lexical_cast<int>(*beg);
                if (datetime[i] < 0)
                    datetime[i] = 0;
            }

            atm.tm_year = datetime[0] > 1900 ? (datetime[0] - 1900) : (datetime[0] + 2000 - 1900);  // tm_year is 1900 based
            atm.tm_mon = datetime[1] > 0 ? (datetime[1] - 1) : 0;                                   // tm_mon is 0 based
            atm.tm_mday = datetime[2] > 0 ? datetime[2] : 1;
            atm.tm_hour = datetime[3] > 0 ? datetime[3] : 0;
            atm.tm_min = datetime[4] > 0 ? datetime[4] : 0;
            atm.tm_sec = datetime[5] > 0 ? datetime[5] : 0;
        }
        catch (const std::exception& e)
        {
            ptime now = second_clock::local_time();
            atm = to_tm(now);
        }
    }
    else
    {
        ptime now = second_clock::local_time();
        atm = to_tm(now);
        gregorianISOParser(dataStr, atm);
    }
}

int64_t Utilities::convertDate(const izenelib::util::UString& dateStr,const izenelib::util::UString::EncodingType& encoding, izenelib::util::UString& outDateStr)
{
    std::string datestr;
    dateStr.convertString(datestr, encoding);
    struct tm atm;
    convert(datestr, atm);
    char str[15];
    memset(str, 0, 15);

    strftime(str, 14, "%Y%m%d%H%M%S", &atm);

    outDateStr.assign(str, encoding);
#ifdef WIN32
    return _mktime64(&atm);
#else
    //return mktime(&atm);
    return fastmktime(&atm);
#endif
}

int64_t Utilities::convertDate(const std::string& dataStr)
{
    struct tm atm;
    convert(dataStr, atm);
#ifdef WIN32
    return _mktime64(&atm);
#else
    //return mktime(&atm);
    return fastmktime(&atm);
#endif

}

time_t Utilities::createTimeStamp()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

time_t Utilities::createTimeStamp(ptime pt)
{
    if (pt.is_not_a_date_time())
        return -1;

    static const ptime epoch(date(1970, 1, 1));
    return (pt - epoch).total_microseconds() + timezone * 1000000;
}

time_t Utilities::createTimeStamp(const izenelib::util::UString& text)
{
    std::string str;
    text.convertString(str, izenelib::util::UString::UTF_8);
    return createTimeStamp(str);
}

// Return -1 when empty, -2 when invalid.
time_t Utilities::createTimeStamp(const string& text)
{
    if (text.empty()) return -1;
    date date_time;
    try
    {
        date_time = from_string(text);
    }
    catch (const std::exception& ex)
    {
    }
    if (!date_time.is_not_a_date())
    {
        return createTimeStamp(ptime(date_time));
    }
    if (text.length() < 8) return -2;
    std::string cand_text = text.substr(0, 8);
    try
    {
        date_time = from_undelimited_string(cand_text);
    }
    catch (const std::exception& ex)
    {
    }
    if (!date_time.is_not_a_date())
    {
        return createTimeStamp(ptime(date_time));
    }
    return -2;
}

bool Utilities::convertPropertyDataType(const std::string& property_name, const PropertyDataType& sf1r_type, izenelib::ir::indexmanager::PropertyType& type)
{
    std::string p = boost::algorithm::to_lower_copy(property_name);
    if (p == "date")
    {
        type = int64_t(0);
        return true;
    }
    if (sf1r_type == STRING_PROPERTY_TYPE)
    {
        type = izenelib::util::UString("", izenelib::util::UString::UTF_8);
        return true;
    }
    else if (sf1r_type == INT_PROPERTY_TYPE)
    {
        type = int64_t(0);
        return true;
    }
    else if (sf1r_type == FLOAT_PROPERTY_TYPE)
    {
        type = float(0.0);
        return true;
    }
    else
    {
        return false;
    }
}

std::string Utilities::uint128ToUuid(const uint128_t& val)
{
    const boost::uuids::uuid& uuid = *reinterpret_cast<const boost::uuids::uuid *>(&val);
    return boost::uuids::to_string(uuid);
}

uint128_t Utilities::uuidToUint128(const std::string& str)
{
    boost::uuids::uuid uuid = boost::lexical_cast<boost::uuids::uuid>(str);
    return *reinterpret_cast<uint128_t *>(&uuid);
}

std::string Utilities::uint128ToMD5(const uint128_t& val)
{
    static char str[33];

    sprintf(str, "%016llx%016llx", (unsigned long long) (val >> 64), (unsigned long long) val);
    return std::string(reinterpret_cast<const char *>(str), 32);

}

uint128_t Utilities::md5ToUint128(const std::string& str)
{
    unsigned long long high, low;

    sscanf(str.c_str(), "%016llx%016llx", &high, &low);
    return (uint128_t) high << 64 | (uint128_t) low;
}

}

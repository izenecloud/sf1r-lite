#include "Utilities.h"

#include <util/hashFunction.h>
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

ptime gregorianISO8601Parser(const std::string& dataStr)
{
    try
    {
        if(dataStr.find(" ") != std::string::npos)
        {
            return ptime(time_from_string(dataStr));
        }
        else if (dataStr.length() == 15 && dataStr.find("T") != std::string::npos)
        {
            return ptime(from_iso_string(dataStr));
        }
        else
        {
            return ptime(from_simple_string(dataStr));
        }
    }
    catch (const std::exception& e)
    {
        if (dataStr.find("/") != std::string::npos)
        {
            try
            {
                std::vector<std::string> ps = split(dataStr, "/");
                size_t len = ps.size();
                if (len == 3)
                {
                    /// format 10/02/2009
                    size_t year = boost::lexical_cast<size_t>(ps[len - 1]);
                    size_t month = boost::lexical_cast<size_t>(ps[len - 2]);
                    size_t day = boost::lexical_cast<size_t>(ps[len - 3]);
                    return ptime(date(year,month,day));
                }
            }
            catch (const std::exception& e){}
        }
        return ptime(date(1970,1,1));
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

ptime convert(const std::string& dataStr)
{
    if (dataStr.size() == 14 && isAllDigit(dataStr))
    {
        /// format 20091009163011
        try
        {
            int offsets[] = {4, 2, 2, 2, 2, 2};

            tm atm;
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
            return ptime(date_from_tm(atm), time_duration(atm.tm_hour, atm.tm_min, atm.tm_sec));
        }
        catch (const std::exception& e)
        {
            return ptime(date(1970,1,1));
        }
    }
    else
    {
        try{
            /// format 2009-10-02
            /// format 2009-10-02 23:12:21
            /// format 2009/10/02
            /// format 2009/10/02 23:12:21
            /// format 10/02/2009
            return gregorianISO8601Parser(dataStr);
        }catch (const std::exception& e)
        {
            return ptime(date(1970,1,1));
        }
    }
}

time_t Utilities::createTimeStampInSeconds(const izenelib::util::UString& dateStr,const izenelib::util::UString::EncodingType& encoding, izenelib::util::UString& outDateStr, bool is_UTC)
{
    std::string datestr;
    dateStr.convertString(datestr, encoding);
    ptime pt = convert(datestr);
    outDateStr.assign(to_iso_string(pt).erase(8, 1), encoding);
    static const ptime epoch(date(1970, 1, 1));
    if (!is_UTC)
        return (pt - epoch).total_seconds() + timezone;
    return (pt - epoch).total_seconds();
}

time_t Utilities::createTimeStampInSeconds(const izenelib::util::UString& text, bool is_UTC)
{
    std::string str;
    text.convertString(str, izenelib::util::UString::UTF_8);
    return createTimeStampInSeconds(str, is_UTC);
}

time_t Utilities::createTimeStampInSeconds(const std::string& dataStr, bool is_UTC)
{
    ptime pt = convert(dataStr);
    static const ptime epoch(date(1970, 1, 1));
    if (!is_UTC)
        return (pt - epoch).total_seconds() + timezone;
    return (pt - epoch).total_seconds();
}

time_t Utilities::createTimeStamp()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

time_t Utilities::createTimeStamp(const ptime& pt, bool is_UTC)
{
    if (pt.is_not_a_date_time())
        return -1;

    static const ptime epoch(date(1970, 1, 1));
    if (!is_UTC)
        return (pt - epoch).total_microseconds() + timezone*1000000;
    return (pt - epoch).total_microseconds();
}

time_t Utilities::createTimeStamp(const izenelib::util::UString& text, bool is_UTC)
{
    std::string str;
    text.convertString(str, izenelib::util::UString::UTF_8);
    return createTimeStamp(str, is_UTC);
}

// Return -1 when empty, -2 when invalid.
time_t Utilities::createTimeStamp(const string& text, bool is_UTC)
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
        return createTimeStamp(ptime(date_time), is_UTC);
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
        return createTimeStamp(ptime(date_time), is_UTC);
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
    else if (sf1r_type == INT32_PROPERTY_TYPE
            || sf1r_type == INT8_PROPERTY_TYPE
            || sf1r_type == INT16_PROPERTY_TYPE)
    {
        type = int32_t(0);
        return true;
    }
    else if (sf1r_type == INT64_PROPERTY_TYPE)
    {
        type = int64_t(0);
        return true;
    }
    else if (sf1r_type == FLOAT_PROPERTY_TYPE)
    {
        type = float(0.0);
        return true;
    }
    else if (sf1r_type == DATETIME_PROPERTY_TYPE)
    {
        type = int64_t(0);
        return true;
    }
    else
    {
        return false;
    }
}

std::string Utilities::uint128ToUuid(const uint128_t& val)
{
    //const boost::uuids::uuid& uuid = *reinterpret_cast<const boost::uuids::uuid *>(&val);
    //return boost::uuids::to_string(uuid);

    static char tmpstr[33];
    sprintf(tmpstr, "%016llx%016llx", (unsigned long long) (val >> 64), (unsigned long long) val);
    return std::string(reinterpret_cast<const char *>(tmpstr), 32);
}

uint128_t Utilities::uuidToUint128(const std::string& str)
{
    if(str.length()==32)
    {
        unsigned long long high = 0, low = 0;
        sscanf(str.c_str(), "%016llx%016llx", &high, &low);
        return (uint128_t) high << 64 | (uint128_t) low;
    }
    else
    {
        boost::uuids::uuid uuid = boost::lexical_cast<boost::uuids::uuid>(str);
        return *reinterpret_cast<uint128_t *>(&uuid);
    }
}

uint128_t Utilities::uuidToUint128(const izenelib::util::UString& ustr)
{
    std::string str;
    ustr.convertString(str, izenelib::util::UString::UTF_8);
    return uuidToUint128(str);
}

std::string Utilities::uint128ToMD5(const uint128_t& val)
{
    static char tmpstr[33];

    sprintf(tmpstr, "%016llx%016llx", (unsigned long long) (val >> 64), (unsigned long long) val);
    return std::string(reinterpret_cast<const char *>(tmpstr), 32);
}

uint128_t Utilities::md5ToUint128(const std::string& str)
{
    if (str.length() != 32)
        return izenelib::util::HashFunction<std::string>::generateHash128(str);

    unsigned long long high = 0, low = 0;

    if (sscanf(str.c_str(), "%016llx%016llx", &high, &low) == 2)
        return (uint128_t) high << 64 | (uint128_t) low;
    else
        return izenelib::util::HashFunction<std::string>::generateHash128(str);
}

uint128_t Utilities::md5ToUint128(const izenelib::util::UString& ustr)
{
    std::string str;
    ustr.convertString(str, izenelib::util::UString::UTF_8);
    return md5ToUint128(str);
}

//void Utilities::uint128ToUuid(const uint128_t& val, std::string& str)
//{
    //const boost::uuids::uuid& uuid = *reinterpret_cast<const boost::uuids::uuid *>(&val);
    //str = boost::uuids::to_string(uuid);
//}

//void Utilities::uuidToUint128(const std::string& str, uint128_t& val)
//{
    //boost::uuids::uuid uuid = boost::lexical_cast<boost::uuids::uuid>(str);
    //val = *reinterpret_cast<uint128_t *>(&uuid);
//}

//void Utilities::uuidToUint128(const izenelib::util::UString& ustr, uint128_t& val)
//{
    //std::string str;
    //ustr.convertString(str, izenelib::util::UString::UTF_8);
    //boost::uuids::uuid uuid = boost::lexical_cast<boost::uuids::uuid>(str);
    //val = *reinterpret_cast<uint128_t *>(&uuid);
//}

//void Utilities::uint128ToMD5(const uint128_t& val, std::string& str)
//{
    //static char tmpstr[33];

    //sprintf(tmpstr, "%016llx%016llx", (unsigned long long) (val >> 64), (unsigned long long) val);
    //str.assign(reinterpret_cast<const char *>(tmpstr), 32);
//}

//void Utilities::md5ToUint128(const std::string& str, uint128_t& val)
//{
    //unsigned long long high = 0, low = 0;

    //sscanf(str.c_str(), "%016llx%016llx", &high, &low);
    //val = (uint128_t) high << 64 | (uint128_t) low;
//}

}

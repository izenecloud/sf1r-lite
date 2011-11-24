#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/token_iterator.hpp>
#include <boost/lexical_cast.hpp>

#include "Utilities.h"

#include <ir/index_manager/utility/StringUtils.h>

#include <vector>

using namespace sf1r;
using namespace izenelib::ir::indexmanager;

void gregorianISO8601Parser(const std::string& dataStr, struct tm& atm)
{
    using namespace boost::gregorian;
    using namespace boost::posix_time;
    try
    {
        if(dataStr.find(" ") != string::npos)
        {
            ptime t(time_from_string(dataStr));
            atm = to_tm(t);
        }
        else
        {
            date d(from_simple_string(dataStr));
            atm = to_tm(d);
        }
    }catch(std::exception& e)
    {
        ptime now = second_clock::local_time();
        atm = to_tm(now);

        if(dataStr.find("/") != string::npos)
        {
            std::vector<std::string> ps = split(dataStr,"/");
            size_t len = ps.size();
            if(len == 3)
            {
                size_t year = boost::lexical_cast<size_t>(ps[len-1]);
                size_t month = boost::lexical_cast<size_t>(ps[len-2]);
                size_t day = boost::lexical_cast<size_t>(ps[len-3]);
                if (year > 1900 && month <= 12 && month > 0 && day <=31 && day >0)
                {
                    /// format 10/02/2009
                    atm.tm_year = year > 1900 ? (year-1900):(year+2000-1900);   // tm_year is 1900 based
                    atm.tm_mon = month >= 0 ? month:0;                          // tm_mon is 0 based
                    atm.tm_mday = day >0 ? day:1;
                }
            }
        }
    }
}

void gregorianISOParser(const std::string& dataStr, struct tm& atm)
{
    using namespace boost::gregorian;
    using namespace boost::posix_time;

    try
    {
        date d(from_undelimited_string(dataStr));
        date::ymd_type ymd = d.year_month_day();
        size_t year = ymd.year;
        size_t month = ymd.month;
        size_t day = ymd.day;

        atm.tm_year = year > 1900 ? (year-1900):(year+2000-1900);   // tm_year is 1900 based
        atm.tm_mon = month >= 0 ? month:0;                          // tm_mon is 0 based
        atm.tm_mday = day >0 ? day:1;

    }catch(std::exception& e)
    {
        ptime now = second_clock::local_time();
        atm = to_tm(now);
    }
}

bool isAllDigit(const string& str)
{
    size_t i ;
    for(i = 0; i != str.length(); i++)
    {
        if(!isdigit(str[i]))
        {
            return false;
        }
    }
    return true;
}

void convert(const std::string& dataStr, struct tm& atm)
{
    using namespace boost::posix_time;
    using namespace boost::gregorian;

    if((dataStr.find("/") != string::npos)||(dataStr.find("-") != string::npos))
    {
        /// format 2009-10-02
        /// format 2009-10-02 23:12:21
        /// format 2009/10/02
        /// format 2009/10/02 23:12:21
        /// format 10/02/2009
        gregorianISO8601Parser(dataStr,atm);
    }
    else if((dataStr.size() == 14)&&(isAllDigit(dataStr)))
    {
        /// format 20091009163011
        int offsets[] = {4,2,2,2,2,2};

        boost::offset_separator f(offsets, offsets+5);
        typedef boost::token_iterator_generator<boost::offset_separator>::type Iter;
        Iter beg = boost::make_token_iterator<string>(dataStr.begin(),dataStr.end(),f);
        Iter end = boost::make_token_iterator<string>(dataStr.end(),dataStr.end(),f);
        int datetime[6];
        memset(datetime,0,6*sizeof(int));
        for(int i = 0;beg!=end;++beg,++i)
        {
            datetime[i] = boost::lexical_cast<int>(*beg);
            if(datetime[i] < 0)
                datetime[i] = 0;
        }

        atm.tm_year = datetime[0] > 1900? (datetime[0]-1900):(datetime[0]+2000-1900);   // tm_year is 1900 based
        atm.tm_mon = datetime[1] > 0? (datetime[1]-1):0;                                // tm_mon is 0 based
        atm.tm_mday = datetime[2] > 0?datetime[2]:1;
        atm.tm_hour = datetime[3] > 0?datetime[3]:0;
        atm.tm_min = datetime[4] > 0?datetime[4]:0;
        atm.tm_sec = datetime[5] > 0?datetime[5]:0;
    }
    else
    {
        ptime now = second_clock::local_time();
        atm = to_tm(now);
        gregorianISOParser(dataStr,atm);
    }
}

int64_t Utilities::convertDate(const izenelib::util::UString& dateStr,const izenelib::util::UString::EncodingType& encoding, izenelib::util::UString& outDateStr)
{
    std::string datestr("");
    dateStr.convertString( datestr, encoding);
    struct tm atm;
    convert(datestr, atm);
    char str[15];
    memset(str,0,15);

    sprintf(str,"%04d%02d%02d%02d%02d%02d",atm.tm_year+1900,atm.tm_mon+1,atm.tm_mday, atm.tm_hour,atm.tm_min,atm.tm_sec);

    outDateStr.assign(str,encoding);
#ifdef WIN32
    return _mktime64(&atm);
#else
    return mktime(&atm);
#endif
}

int64_t Utilities::convertDate(const std::string& dataStr)
{
    struct tm atm;
    convert(dataStr,atm);
#ifdef WIN32
    return _mktime64(&atm);
#else
    return mktime(&atm);
#endif

}

std::string Utilities::toLower(const std::string& str)
{
    std::string szResp(str);
    std::transform(szResp.begin(),szResp.end(), szResp.begin(), (int(*)(int))std::tolower);
    return szResp;
}

std::string Utilities::toUpper(const std::string& str)
{
    std::string szResp(str);
    std::transform(szResp.begin(),szResp.end(), szResp.begin(), (int(*)(int))std::toupper);
    return szResp;
}

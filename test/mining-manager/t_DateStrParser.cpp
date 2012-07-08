///
/// @file t_DateStrParser.cpp
/// @brief test DateStrParser in parsing between date and string
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2012-07-04
///

#include <mining-manager/group-manager/DateStrParser.h>
#include <boost/test/unit_test.hpp>

using namespace sf1r::faceted;

namespace
{

DateStrParser& DATE_STR_PARSER = *DateStrParser::get();

void validUnitStrToMask(
    const std::string& unitStr,
    DATE_MASK_TYPE goldMask
)
{
    DATE_MASK_TYPE mask;
    std::string errorMsg;

    BOOST_CHECK(DATE_STR_PARSER.unitStrToMask(unitStr, mask, errorMsg));
    BOOST_CHECK_EQUAL(mask, goldMask);
}

void invalidUnitStrToMask(const std::string& unitStr)
{
    DATE_MASK_TYPE mask;
    std::string errorMsg;

    BOOST_CHECK(! DATE_STR_PARSER.unitStrToMask(unitStr, mask, errorMsg));
}

void validAPIStrToDateMask(
    const std::string& dateStrIn,
    DATE_MASK_TYPE goldMask
)
{
    DateStrParser::DateMask dateMask;
    std::string errorMsg;

    BOOST_CHECK(DATE_STR_PARSER.apiStrToDateMask(dateStrIn, dateMask, errorMsg));
    BOOST_CHECK_EQUAL(dateMask.mask_, goldMask);

    std::string dateStrOut;
    DATE_STR_PARSER.dateToAPIStr(dateMask.date_, dateStrOut);
    BOOST_CHECK_EQUAL(dateStrOut, dateStrIn);
}

void invalidAPIStrToDateMask(const std::string& dateStr)
{
    DateStrParser::DateMask dateMask;
    std::string errorMsg;

    BOOST_CHECK(! DATE_STR_PARSER.apiStrToDateMask(dateStr, dateMask, errorMsg));
}

void validScdStrToDate(
    const std::string& scdStr,
    const std::string& goldDateStr
)
{
    DateGroupTable::date_t date = 0;
    std::string errorMsg;

    BOOST_CHECK(DATE_STR_PARSER.scdStrToDate(scdStr, date, errorMsg));

    std::string dateStrOut;
    DATE_STR_PARSER.dateToAPIStr(date, dateStrOut);
    BOOST_CHECK_EQUAL(dateStrOut, goldDateStr);
}

void invalidScdStrToDate(const std::string& scdStr)
{
    DateGroupTable::date_t date = 0;
    std::string errorMsg;

    BOOST_CHECK(! DATE_STR_PARSER.scdStrToDate(scdStr, date, errorMsg));
}

}

BOOST_AUTO_TEST_SUITE(DateStrParserTest)

BOOST_AUTO_TEST_CASE(testUnitStrToMask)
{
    validUnitStrToMask("Y", MASK_YEAR);
    validUnitStrToMask("M", MASK_YEAR_MONTH);
    validUnitStrToMask("D", MASK_YEAR_MONTH_DAY);

    invalidUnitStrToMask("");
    invalidUnitStrToMask("y");
    invalidUnitStrToMask("other");
}

BOOST_AUTO_TEST_CASE(testAPIStrToDateMask)
{
    validAPIStrToDateMask("0000", MASK_YEAR);
    validAPIStrToDateMask("2012", MASK_YEAR);
    validAPIStrToDateMask("2012-07", MASK_YEAR_MONTH);
    validAPIStrToDateMask("2012-07-04", MASK_YEAR_MONTH_DAY);
    validAPIStrToDateMask("9999-12-31", MASK_YEAR_MONTH_DAY);

    invalidAPIStrToDateMask("");
    invalidAPIStrToDateMask("abc");
    invalidAPIStrToDateMask("10000");
    invalidAPIStrToDateMask("2012-0");
    invalidAPIStrToDateMask("2012-13");
    invalidAPIStrToDateMask("2012-11-0");
    invalidAPIStrToDateMask("2012-11-32");
    invalidAPIStrToDateMask("2012-11-12T14:30:05");
    invalidAPIStrToDateMask("20121132");
    invalidAPIStrToDateMask("20121112T143005");
    invalidAPIStrToDateMask("20121112143005");
}

BOOST_AUTO_TEST_CASE(testScdStrToDate)
{
    validScdStrToDate("20121112143005", "2012-11-12");
    validScdStrToDate("20130102030405", "2013-01-02");

    invalidScdStrToDate("");
    invalidScdStrToDate("2012");
    invalidScdStrToDate("20121011");
    invalidScdStrToDate("2012-10-11");
    invalidScdStrToDate("2012/10/11");
    invalidScdStrToDate("2012-10-11T14:30:05");
    invalidScdStrToDate("2012-10-11 14:30:05");
    invalidScdStrToDate("20121011T143005");
    invalidScdStrToDate("abc");
}

BOOST_AUTO_TEST_SUITE_END() 

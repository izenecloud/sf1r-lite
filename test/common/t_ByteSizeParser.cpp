/**
 * @file t_ByteSizeParser.cpp
 * @brief test ByteSizeParser
 */

#include <common/ByteSizeParser.h>
#include <boost/test/unit_test.hpp>

namespace
{

template <typename IntT>
void checkParseNormal(const std::string& str, IntT gold)
{
    sf1r::ByteSizeParser* parser = sf1r::ByteSizeParser::get();

    IntT actual = parser->parse<IntT>(str);
    BOOST_CHECK_EQUAL(actual, gold);
}

template <typename IntT>
void checkParseException(const std::string& str)
{
    sf1r::ByteSizeParser* parser = sf1r::ByteSizeParser::get();

    try
    {
        IntT actual = parser->parse<IntT>(str);
        BOOST_ERROR("expect exception for " << str <<
                    ", while got parse result " << actual);
    }
    catch (const std::exception& e)
    {
        BOOST_TEST_MESSAGE("given " << str <<
                           ", exception: " << e.what());
    }
}

void checkFormat(uint64_t value, const std::string& gold)
{
    sf1r::ByteSizeParser* parser = sf1r::ByteSizeParser::get();

    std::string actual = parser->format(value);
    BOOST_CHECK_EQUAL(actual, gold);
}

}

BOOST_AUTO_TEST_SUITE(ByteSizeParserTest)

BOOST_AUTO_TEST_CASE(testParseNormal)
{
    checkParseNormal("0", 0);
    checkParseNormal("1", 1);
    checkParseNormal("123456", 123456);
    checkParseNormal("2147483647", 2147483647);
    checkParseNormal("2147483648", 2147483648);

    checkParseNormal("0g", 0);
    checkParseNormal("5b", 5);
    checkParseNormal("5B", 5);
    checkParseNormal("2k", 2048);
    checkParseNormal("1m", 1048576);
    checkParseNormal("256m", 268435456);
    checkParseNormal("1gb", 1073741824);
    checkParseNormal("1t", 1099511627776U);
    checkParseNormal("1PB", 1125899906842624U);
    checkParseNormal("1eb", 1152921504606846976U);
    checkParseNormal("15eb", 17293822569102704640U);
    checkParseNormal("18446744073709551615", 18446744073709551615U);

    checkParseNormal("12.3456", 12);
    checkParseNormal("1.5k", 1536);
    checkParseNormal("1.m", 1048576);
    checkParseNormal("215.8901m", 226377177);
    checkParseNormal("10.30GB", 11059540787U);
    checkParseNormal("0.46tB", 505775348776U);

    checkParseNormal(" 5.2  mb  ", 5452595);
    checkParseNormal("\t\n  3.3  kb\n\t  ", 3379);
}

BOOST_AUTO_TEST_CASE(testParseException)
{
    checkParseException<int>("");
    checkParseException<int>("gb");
    checkParseException<int>("K");
    checkParseException<int>(".9m");
    checkParseException<int>("-3gb");
    checkParseException<int>("t1s5");
    checkParseException<int>("abc");
    checkParseException<int>("10FF");
    checkParseException<int>("中国");

    checkParseException<int>(" 5 .2  mb");
    checkParseException<int>("1BC");
    checkParseException<int>("1-k");
    checkParseException<int>("1.2.3");

    checkParseException<uint8_t>("1k");
    checkParseException<uint16_t>("1MB");
    checkParseException<int32_t>("2g");
    checkParseException<uint32_t>("4GB");
    checkParseException<int32_t>("2147483648");

    checkParseException<uint64_t>("16EB");
    checkParseException<uint64_t>("20000p");
    checkParseException<uint64_t>("18446744073709551616");
    checkParseException<uint64_t>("11111111111111111111111");
    checkParseException<uint64_t>("9999999999999999999999999");
}

BOOST_AUTO_TEST_CASE(testFormat)
{
    checkFormat(0, "0");
    checkFormat(1, "1");
    checkFormat(78, "78");
    checkFormat(1023, "1023");

    checkFormat(1024, "1.0K");
    checkFormat(1025, "1.0K");
    checkFormat(1076, "1.1K");
    checkFormat(8200, "8.0K");
    checkFormat(1000000, "977K");
    checkFormat(1047552, "1023K");
    checkFormat(1047555, "1023K");

    checkFormat(1048576, "1.0M");
    checkFormat(97517600, "93M");
    checkFormat(126877696, "121M");

    checkFormat(1400000000, "1.3G");
    checkFormat(9100000000U, "8.5G");
    checkFormat(35500000000U, "33G");

    checkFormat(6.2e12, "5.6T");
    checkFormat(1.3e16, "12P");
    checkFormat(2.8e18, "2.4E");
    checkFormat(-1, "16E");
}

BOOST_AUTO_TEST_SUITE_END()

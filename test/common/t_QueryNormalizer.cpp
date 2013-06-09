/**
 * @file t_QueryNormalizer.cpp
 * @brief test normalizing the query string
 */

#include <common/QueryNormalizer.h>
#include <boost/test/unit_test.hpp>

namespace
{

void checkNormalize(const std::string& source, const std::string& expect)
{
    std::string actual;
    sf1r::QueryNormalizer::get()->normalize(source, actual);

    BOOST_CHECK_EQUAL(actual, expect);
}

void checkCharNum(const std::string& source, std::size_t expect)
{
    std::size_t actual = sf1r::QueryNormalizer::get()->countCharNum(source);

    BOOST_TEST_MESSAGE("source: " << source <<
                       ", expect char num: " << expect <<
                       ", actual char num: " << actual);

    BOOST_CHECK_EQUAL(actual, expect);
}

void checkLongQuery(const std::string& source, bool expect)
{
    bool actual = sf1r::QueryNormalizer::get()->isLongQuery(source);

    BOOST_CHECK_EQUAL(actual, expect);
}

}

BOOST_AUTO_TEST_SUITE(QueryNormalizerTest)

BOOST_AUTO_TEST_CASE(testEmptyOutput)
{
    std::string source;
    const std::string empty;

    checkNormalize(source, empty);
    checkCharNum(source, 0);
    checkLongQuery(source, false);

    source = "  \t   \n";
    checkNormalize(source, empty);
    checkCharNum(source, 0);
    checkLongQuery(source, false);
}

BOOST_AUTO_TEST_CASE(testTokenizeByWhiteSpace)
{
    std::string source("aa bb cc dd ee");
    std::string expect(source);
    checkNormalize(source, expect);
    checkCharNum(source, 5);
    checkLongQuery(source, false);

    source = " aa  bb   cc\tdd\n\ree\n";
    checkNormalize(source, expect);
    checkCharNum(source, 5);
    checkLongQuery(source, false);
}

BOOST_AUTO_TEST_CASE(testConvertToLowerCase)
{
    checkNormalize("AAA", "aaa");
    checkNormalize("AA BB cc x Y Z", "aa bb cc x y z");
}

BOOST_AUTO_TEST_CASE(testSortByLexicalOrder)
{
    checkNormalize("k UU III", "iii k uu");
    checkNormalize("cc Z x AA BB Y", "aa bb cc x y z");

    std::string source("HTC one M7");
    checkNormalize(source, "htc m7 one");
    checkCharNum(source, 3);
    checkLongQuery(source, false);
}

BOOST_AUTO_TEST_CASE(testMixInput)
{
    std::string source("iPhone5手机");
    checkNormalize(source, "iphone5手机");
    checkCharNum(source, 3);
    checkLongQuery(source, false);

    source = "iPhone 5 手机";
    checkNormalize(source, "5 iphone 手机");
    checkCharNum(source, 4);
    checkLongQuery(source, false);

    source = "polo衫 短袖\t男\r\n";
    checkNormalize(source, "polo衫 男 短袖");
    checkCharNum(source, 5);
    checkLongQuery(source, false);

    source = "  三星  Galaxy  I9300  ";
    checkNormalize(source, "galaxy i9300 三星");
    checkCharNum(source, 4);
    checkLongQuery(source, false);

    source = "5.5寸手机 1.8 NO.1 P7";
    checkNormalize(source, "1.8 5.5寸手机 no.1 p7");
    checkCharNum(source, 7);
    checkLongQuery(source, false);

    source = "Haier/海尔 ES50H-D1(E) 50升";
    checkNormalize(source, "50升 es50h-d1(e) haier/海尔");
    checkCharNum(source, 8);
    checkLongQuery(source, true);

    source = "O.P.I 天然 -80# 珍珠色 L’OREAL/欧莱雅";
    checkNormalize(source, "-80# l’oreal/欧莱雅 o.p.i 天然 珍珠色");
    checkCharNum(source, 12);
    checkLongQuery(source, true);
}

BOOST_AUTO_TEST_SUITE_END()

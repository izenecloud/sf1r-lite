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
    sf1r::QueryNormalizer::normalize(source, actual);

    BOOST_CHECK_EQUAL(actual, expect);
}

}

BOOST_AUTO_TEST_SUITE(QueryNormalizerTest)

BOOST_AUTO_TEST_CASE(testEmptyOutput)
{
    std::string empty;
    checkNormalize(empty, empty);
    checkNormalize("  \t   \n", empty);
}

BOOST_AUTO_TEST_CASE(testTokenizeByWhiteSpace)
{
    std::string expect("aa bb cc dd ee");
    checkNormalize(expect, expect);
    checkNormalize(" aa  bb   cc\tdd\n\ree\n", expect);
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
    checkNormalize("HTC one M7", "htc m7 one");
}

BOOST_AUTO_TEST_CASE(testMixInput)
{
    checkNormalize("iPhone5手机", "iphone5手机");
    checkNormalize("iPhone 5 手机", "5 iphone 手机");

    checkNormalize("polo衫 短袖\t男\r\n", "polo衫 男 短袖");
    checkNormalize("  三星  Galaxy  I9300  ", "galaxy i9300 三星");

    checkNormalize("O.P.I 天然 -80# 珍珠色 L’OREAL/欧莱雅",
                   "-80# l’oreal/欧莱雅 o.p.i 天然 珍珠色");
}

BOOST_AUTO_TEST_SUITE_END()

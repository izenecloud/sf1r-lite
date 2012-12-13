/**
 * @file t_ProductRanker.cpp
 * @brief test ProductRanker to rank topK products by below factors:
 * 1. category score
 * 2. merchant count
 * 3. diversity round
 * @author Jun Jiang
 * @date Created 2012-12-06
 */

#include "ProductRankerTestFixture.h"
#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(ProductRankerTest,
                         sf1r::ProductRankerTestFixture)

BOOST_AUTO_TEST_CASE(testDiversity)
{
    BOOST_TEST_MESSAGE("test merchant diversity alone");

    setDocId(             "1    2    3    4    5    6    7    8    9");
    setTopKScore(         "0.1  0.2  0.3  0.4  0.5  0.6  0.7  0.8  0.9");
    setMultiDocMerchantId("1    1    1    2    2    2    3    3    3");
    //diversity round:     1    2    3    1    2    3    1    2    3

    rank();

    //diversity round:     1    1    1    2    2    2    3    3    3
    checkDocId(           "1    4    7    2    5    8    3    6    9");
    checkTopKScore(       "0.1  0.4  0.7  0.2  0.5  0.8  0.3  0.6  0.9");
}

BOOST_AUTO_TEST_CASE(testAllFactors)
{
    BOOST_TEST_MESSAGE("test all ranking factors");

    setDocId(             "1     2    3    4    5    6    7    8    9    10");
    setTopKScore(         "12.1  1.2  0.3  0.4  0.5  0.6  0.7  0.8  0.9  0.1");
    setMultiDocMerchantId("1     1    1    2    2    3    3    2    1");
    setSingleDocMerchantId("1 3");
    //diversity round:     0     0    1    1    2    1    2    3    2    0

    rank();

    //diversity round:     0     0    0    1    1    1    2    2    2    3
    checkDocId(           "1     2    10   3    4    6    5    7    9    8");
    checkTopKScore(       "12.1  1.2  0.1  0.3  0.4  0.6  0.5  0.7  0.9  0.8");
}

BOOST_AUTO_TEST_CASE(testRandomScore)
{
    BOOST_TEST_MESSAGE("test random score");

    configRandomScore();

    setDocId(             "1     2    3    4    5    6    7    8    9");
    setTopKScore(         "12.1  1.2  0.3  0.4  0.5  0.6  0.7  0.8  0.9");

    rank();

    int equalNum = 2;
    checkDocId(           "1     2    3    4    5    6    7    8    9",
                          equalNum);
    checkTopKScore(       "12.1  1.2  0.3  0.4  0.5  0.6  0.7  0.8  0.9",
                          equalNum);
}

BOOST_AUTO_TEST_SUITE_END()

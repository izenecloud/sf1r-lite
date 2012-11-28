/**
 * @file t_NumericPropertyScorer.cpp
 * @brief test NumericPropertyScorer to calculate score for numeric value.
 * @author Jun Jiang
 * @date Created 2012-11-26
 */

#include "NumericPropertyScorerTestFixture.h"
#include <boost/test/unit_test.hpp>

using namespace sf1r;

BOOST_FIXTURE_TEST_SUITE(NumericPropertyScorerTest,
                         NumericPropertyScorerTestFixture)

BOOST_AUTO_TEST_CASE(emptyNumericValue)
{
    BOOST_TEST_MESSAGE("check empty numeric value");

    setNumericValue("");
    createScorer();

    score_t gold = 0;
    docid_t maxDocId = 3;
    for (docid_t docId = 1; docId <= maxDocId; ++docId)
    {
        checkScore(docId, gold);
    }
}

BOOST_AUTO_TEST_CASE(oneNumericValue)
{
    BOOST_TEST_MESSAGE("check one numeric value");

    setNumericValue("5");
    createScorer();

    score_t gold = 0;
    docid_t maxDocId = 3;
    for (docid_t docId = 1; docId <= maxDocId; ++docId)
    {
        checkScore(docId, gold);
    }
}

BOOST_AUTO_TEST_CASE(twoEqualNumericValues)
{
    BOOST_TEST_MESSAGE("check two equal numeric values");

    setNumericValue("3 3");
    createScorer();

    score_t gold = 0;
    docid_t maxDocId = 3;
    for (docid_t docId = 1; docId <= maxDocId; ++docId)
    {
        checkScore(docId, gold);
    }
}

BOOST_AUTO_TEST_CASE(twoDiffNumericValues)
{
    BOOST_TEST_MESSAGE("check two different numeric values");

    setNumericValue("1 3"); // max - min = 2
    createScorer();

    checkScore(1, (1.0-1) / 2);
    checkScore(2, (3.0-1) / 2);
    checkScore(3, 0);   // no numeric value
}

BOOST_AUTO_TEST_CASE(threeNumericValues)
{
    BOOST_TEST_MESSAGE("check three numeric values");

    setNumericValue("3 1 2"); // max - min = 2
    createScorer();

    checkScore(1, (3.0-1) / 2);
    checkScore(2, (1.0-1) / 2);
    checkScore(3, (2.0-1) / 2);
    checkScore(4, 0);   // no numeric value

    // max & min numeric values have been fixed in createScorer()
    setNumericValue("0 4 2 3");

    checkScore(1, 0);   // (0-1) / 2 -> min score 0
    checkScore(2, 1);   // (4-1) / 2 -> max score 1
    checkScore(3, (2.0-1) / 2);
    checkScore(4, (3.0-1) / 2);
    checkScore(5, 0);   // no numeric value
}

BOOST_AUTO_TEST_CASE(skipInvalidValues)
{
    BOOST_TEST_MESSAGE("check skipping invalid values");

    setNumericValue("2 3");
    setNumericValue(5, 8);
    setNumericValue(7, 4); // max - min = 6
    createScorer();

    checkScore(1, (2.0-2) / 6);
    checkScore(2, (3.0-2) / 6);
    checkScore(3, 0);   // no numeric value
    checkScore(4, 0);   // no numeric value
    checkScore(5, (8.0-2) / 6);
    checkScore(6, 0);   // no numeric value
    checkScore(7, (4.0-2) / 6);
    checkScore(8, 0);   // no numeric value
}

BOOST_AUTO_TEST_SUITE_END()

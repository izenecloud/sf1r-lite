/**
 * @file t_ProductScoreAverage.cpp
 * @brief test ProductScoreAverage to calculate average score.
 * @author Jun Jiang
 * @date Created 2012-11-23
 */

#include "ProductScoreAverageTestFixture.h"
#include <boost/test/unit_test.hpp>

using namespace sf1r;

namespace
{

const float FLOAT_TOLERANCE = 1e-5;

} // namespace

BOOST_FIXTURE_TEST_SUITE(ProductScoreAverageTest,
                         ProductScoreAverageTestFixture)

BOOST_AUTO_TEST_CASE(emptyScorer)
{
    BOOST_TEST_MESSAGE("check empty scorer");

    addScorers("", "");

    docid_t docId = 1;
    score_t gold = 0;
    score_t result = averageScorer_->score(docId);

    BOOST_CHECK_EQUAL(result, gold);
}

BOOST_AUTO_TEST_CASE(oneScorer)
{
    BOOST_TEST_MESSAGE("check one scorer");

    addScorers("0.8", "0.6");

    docid_t docId = 1;
    score_t gold = 0.8 * 0.6;
    score_t result = averageScorer_->score(docId);

    BOOST_CHECK_CLOSE(result, gold, FLOAT_TOLERANCE);
}

BOOST_AUTO_TEST_CASE(multiScorer)
{
    BOOST_TEST_MESSAGE("check multiple scorers");

    addScorers("0.4 0.3 0.7", "0.2 0.6 0.3");

    docid_t docId = 1;
    score_t gold = (0.4*0.2 + 0.3*0.6 + 0.7*0.3) / 3;
    score_t result = averageScorer_->score(docId);

    BOOST_CHECK_CLOSE(result, gold, FLOAT_TOLERANCE);
}

BOOST_AUTO_TEST_SUITE_END()

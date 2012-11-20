/**
 * @file t_ProductScoreManager.cpp
 * @brief test ProductScoreManager functions.
 * @author Jun Jiang
 * @date Created 2012-11-19
 */

#include "ProductScoreManagerTestFixture.h"
#include <boost/test/unit_test.hpp>
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>

using namespace sf1r;

BOOST_FIXTURE_TEST_SUITE(ProductScoreManagerTest,
                         ProductScoreManagerTestFixture)

BOOST_AUTO_TEST_CASE(checkBuildScore)
{
    BOOST_TEST_MESSAGE("check empty product score");
    checkScore();

    BOOST_TEST_MESSAGE("build product score 1st time");
    buildScore(100);
    checkScore();

    BOOST_TEST_MESSAGE("build product score 2nd time");
    buildScore(200);
    checkScore();

    BOOST_TEST_MESSAGE("load product score");
    resetScoreManager();
    checkScore();

    BOOST_TEST_MESSAGE("build product score 3rd time");
    buildScore(300);
    checkScore();
}

BOOST_AUTO_TEST_CASE(concurrentBuildAndRead)
{
    int docNum = 2000;
    insertDocument(docNum);

    boost::thread_group threads;
    threads.create_thread(
        boost::bind(&ProductScoreManagerTestFixture::buildCollection, this));

    int readThreadNum = 15;
    for (int i = 0; i < readThreadNum; ++i)
    {
        threads.create_thread(
            boost::bind(&ProductScoreManagerTestFixture::readScore,
                        this, docNum));
    }

    threads.join_all();
    checkScore();
}

BOOST_AUTO_TEST_SUITE_END()

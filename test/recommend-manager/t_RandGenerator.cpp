///
/// @file t_RandGenerator.cpp
/// @brief test RandGenerator in generating random ids
/// @author Jun Jiang
/// @date Created 2011-11-30
///

#include <recommend-manager/RandGenerator.h>
#include <recommend-manager/RecTypes.h>

#include <boost/test/unit_test.hpp>
#include <ctime>

using namespace sf1r;
using namespace boost;

typedef itemid_t ID_TYPE;
typedef RandGenerator<ID_TYPE> ItemIdGenerator;

BOOST_AUTO_TEST_SUITE(RandGeneratorTest)

BOOST_AUTO_TEST_CASE(test_generate)
{
    ItemIdGenerator rand;
    const ID_TYPE min = 1;
    const ID_TYPE max = 100;

    for (int i=1; i<20; ++i)
    {
        ID_TYPE id = rand.generate(min, max);
        BOOST_TEST_MESSAGE(id);

        BOOST_CHECK_GE(id, min);
        BOOST_CHECK_LE(id, max);
    }
}

BOOST_AUTO_TEST_CASE(test_seed)
{
    ItemIdGenerator rand;
    rand.seed(std::time(NULL));

    const ID_TYPE min = 1;
    const ID_TYPE max = 100;

    for (int i=1; i<20; ++i)
    {
        ID_TYPE id = rand.generate(min, max);
        BOOST_TEST_MESSAGE(id);

        BOOST_CHECK_GE(id, min);
        BOOST_CHECK_LE(id, max);
    }
}

BOOST_AUTO_TEST_SUITE_END() 

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
#include <stdexcept>

using namespace sf1r;
using namespace boost;

namespace
{

typedef itemid_t ID_TYPE;
typedef RandGenerator<ID_TYPE> ItemIdGenerator;

void checkRand(
    ItemIdGenerator& rand,
    ID_TYPE min, 
    ID_TYPE max
)
{
    const int count = 20;
    for (int i=1; i<count; ++i)
    {
        ID_TYPE id = rand.generate(min, max);
        BOOST_TEST_MESSAGE(id);

        BOOST_CHECK_GE(id, min);
        BOOST_CHECK_LE(id, max);
    }
}

}

BOOST_AUTO_TEST_SUITE(RandGeneratorTest)

BOOST_AUTO_TEST_CASE(test_invalid_range)
{
    ItemIdGenerator rand;
    typedef std::invalid_argument ExpectException;
    BOOST_CHECK_THROW(rand.generate(1, 0), ExpectException);
    BOOST_CHECK_THROW(rand.generate(100, 10), ExpectException);
}

BOOST_AUTO_TEST_CASE(test_generate)
{
    ItemIdGenerator rand;
    checkRand(rand, 1, 100);
}

BOOST_AUTO_TEST_CASE(test_seed)
{
    ItemIdGenerator rand;
    rand.seed(std::time(NULL));
    checkRand(rand, 1, 100);
}

BOOST_AUTO_TEST_SUITE_END() 

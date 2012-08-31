#include <boost/test/unit_test.hpp>

#include <search-manager/CustomRanker.h>

using namespace sf1r;
 
BOOST_AUTO_TEST_SUITE( CustomRanker_Suite )

 
BOOST_AUTO_TEST_CASE(customranker_test)
{
    {
        std::string exp("param1 + param2 * price + log(x)");
        CustomRanker customRanker(exp);
        bool ret = customRanker.parse();
        BOOST_CHECK_EQUAL(ret, true);
    }
    {
        std::string exp("param1 - (param2 / price) * pow(x,2)");
        CustomRanker customRanker(exp);
        bool ret = customRanker.parse();
        BOOST_CHECK_EQUAL(ret, true);
    }
}

BOOST_AUTO_TEST_SUITE_END()


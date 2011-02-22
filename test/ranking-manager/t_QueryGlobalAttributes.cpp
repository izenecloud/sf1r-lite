#include <boost/test/unit_test.hpp>

#include <ranking-manager/QueryGlobalAttributes.h>

using namespace sf1r;

BOOST_AUTO_TEST_SUITE(QueryGlobalAttributes_test)

BOOST_AUTO_TEST_CASE(Ctor_test)
{
    QueryGlobalAttributes attr;
    BOOST_CHECK(attr.getNumDocs() == 0);
    BOOST_CHECK(attr.getTotalPropertyLength() == 0);
    BOOST_CHECK(attr.getAveragePropertyLength() == 0.0F);
    BOOST_CHECK(attr.getQueryLength() == 0);
}


BOOST_AUTO_TEST_CASE(NumDocs_test)
{
    QueryGlobalAttributes attr;
    attr.setNumDocs(10);
    BOOST_CHECK(attr.getNumDocs() == 10);
}

BOOST_AUTO_TEST_CASE(TotalPropertyLength_test)
{
    QueryGlobalAttributes attr;
    attr.setTotalPropertyLength(10);
    BOOST_CHECK(attr.getTotalPropertyLength() == 10);
}

BOOST_AUTO_TEST_CASE(AveragePropertyLength_test)
{
    QueryGlobalAttributes attr;
    attr.setNumDocs(10);
    attr.setTotalPropertyLength(100);
    BOOST_CHECK(attr.getAveragePropertyLength() - 10.0F < 1E-8F);
}

BOOST_AUTO_TEST_CASE(QueryLength_test)
{
    QueryGlobalAttributes attr;
    attr.setQueryLength(10);
    BOOST_CHECK(attr.getQueryLength() == 10);
}

BOOST_AUTO_TEST_SUITE_END() // QueryGlobalAttributes_test


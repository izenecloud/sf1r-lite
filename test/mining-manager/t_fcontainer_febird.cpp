///
/// @file t_fcontainer_febird.cpp
/// @brief test febird serialization on std containers
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2012-06-01
///

#include "FContainerTestFixture.h"
#include <util/ustring/UString.h>

#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>
#include <vector>

using namespace sf1r;

BOOST_FIXTURE_TEST_SUITE(FContainerFebirdTest, FContainerTestFixture)

BOOST_AUTO_TEST_CASE(checkEmpty)
{
    typedef std::vector<int> ContainerT;

    checkEmptyFile<ContainerT>("vector_int.bin");
}

BOOST_AUTO_TEST_CASE(checkVectorInt)
{
    typedef std::vector<int> ContainerT;
    ContainerT container;

    const int num = 1000;
    for (int i=0; i<num; ++i)
    {
        container.push_back(i);
    }

    checkContainerSerialize("vector_int.bin", container);
}

BOOST_AUTO_TEST_CASE(checkVectorUString)
{
    typedef std::vector<izenelib::util::UString> ContainerT;
    ContainerT container;

    const int num = 1000;
    for (int i=0; i<num; ++i)
    {
        std::string str = boost::lexical_cast<std::string>(i);
        izenelib::util::UString ustr(str, izenelib::util::UString::UTF_8);
        container.push_back(ustr);
    }

    checkContainerSerialize("vector_ustr.bin", container);
}

BOOST_AUTO_TEST_SUITE_END() 

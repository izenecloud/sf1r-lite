/**
 * @file TestHelper.h
 *
 * Created      <2008-10-19 17:59:22 Ian Yang>
 * Last Updated <2009-02-27 18:04:05 Ian Yang>
 */
#ifndef INCLUDED_TEST_HELPER_H
#define INCLUDED_TEST_HELPER_H

#define IZS_CHECK_DOUBLE_CLOSE(a, b, e) BOOST_CHECK_SMALL((static_cast<double>(a) - static_cast<double>(b)), static_cast<double>(e))

#include <boost/version.hpp>

#if BOOST_VERSION >= 103500 && BOOST_VERSION <= 103700

#include <boost/test/test_tools.hpp>
#include <boost/test/floating_point_comparison.hpp>

namespace {

struct ignore_unused_variable_warnings_in_boost_test
{
    ignore_unused_variable_warnings_in_boost_test()
    {
        use(boost::test_tools::check_is_small);
        use(boost::test_tools::dummy_cond);
        use(boost::test_tools::check_is_close);
    }

    template<typename T>
    void use(const T&)
    {
        // empty
    }
};

}

#endif

#endif // INCLUDED_TEST_HELPER_H

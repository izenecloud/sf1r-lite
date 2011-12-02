/** 
 * @file t_master_suite.cpp
 * @brief The master test suite for CachePolicyManager
 * @details
 *   This file defines the BOOST_TEST_DYN_LINK and BOOST_TEST_MAIN, which should be defined only once for one master test suite.
 *   It can also have test suites and test cases of the main process.
 * @author MyungHyun (Kent)
 * @date 2008-07-09
 */

/**
 * @brief Needed to use boost shared library. should define before including unit_test.hpp. 
 * Only one define statement should exist in a 
 */
#define BOOST_TEST_DYN_LINK     
/**
 * @brief Needed to use boost shared library. should define before including unit_test.hpp. 
 * Only one define statement should exist in a 
 */
#define BOOST_TEST_MAIN         

/** Can define a test module in the following way. */
#define BOOST_TEST_MODULE CPM test suite

#include <boost/test/unit_test.hpp>


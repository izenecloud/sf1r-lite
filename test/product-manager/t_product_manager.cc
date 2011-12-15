#include <product-manager/simple_data_source.h>
#include <product-manager/simple_operation_processor.h>
#include <product-manager/product_manager.h>
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include "pm_test_runner.h"
using namespace sf1r;

BOOST_AUTO_TEST_SUITE(ProductManager_test)

static PMConfig pm_config;


BOOST_AUTO_TEST_CASE(new_test)
{
//     PMTestRunner runner;
// 
//     {
//         std::vector<std::string> input_list;
//         input_list.push_back("I,1,4.0");
//         input_list.push_back("I,2,4.0-5.0");
//         input_list.push_back("I,3,7.0");
// 
//         std::vector<std::string> result_list;
//         result_list.push_back("1,4.0,1,1");
//         result_list.push_back("1,4.0-5.0,1,2");
//         result_list.push_back("1,7.0,1,3");
//         runner.Test(input_list, result_list);
//     }
// 
//     {
//         std::vector<std::string> input_list;
//         input_list.push_back("U,1,10.0,1,1");
// 
//         std::vector<std::string> result_list;
//         result_list.push_back("2,10.0,1,1");
//         runner.Test(input_list, result_list);
//     }
// 
//     {
//         std::vector<std::string> input_list;
//         input_list.push_back("C,1:2,1");
// 
//         std::vector<std::string> result_list;
//         //HOW to check the del docid(uuid)?
//         result_list.push_back("3,,,");
//         result_list.push_back("2,4.0-10.0,2,1");
// 
//         runner.Test(input_list, result_list);
//     }
// 
//     {
//         std::vector<std::string> input_list;
//         input_list.push_back("C,2:3,0");
// 
//         std::vector<std::string> result_list;
// 
//         runner.Test(input_list, result_list);
//     }
// 
//     {
//         std::vector<std::string> input_list;
//         input_list.push_back("U,2,50.0-60.0,2,1");
// 
//         std::vector<std::string> result_list;
//         result_list.push_back("2,10.0-60.0,,1");
//         runner.Test(input_list, result_list);
//     }
// 
//     {
//         std::vector<std::string> input_list;
//         input_list.push_back("I,4,");
// 
//         std::vector<std::string> result_list;
//         result_list.push_back("1,,1,4");
//         runner.Test(input_list, result_list);
//     }
// 
//     {
//         std::vector<std::string> input_list;
//         input_list.push_back("A,1,4,1");
// 
//         std::vector<std::string> result_list;
//         result_list.push_back("3,,,");
//         result_list.push_back("2,10.0-60.0,3,1");
//         runner.Test(input_list, result_list);
//     }
// 
//     {
//         std::vector<std::string> input_list;
//         input_list.push_back("R,1,2,1");
// 
//         std::vector<std::string> result_list;
//         result_list.push_back("2,10.0,2,1");
//         result_list.push_back("1,50.0-60.0,1,");
// 
//         runner.Test(input_list, result_list);
//     }

}

BOOST_AUTO_TEST_SUITE_END()

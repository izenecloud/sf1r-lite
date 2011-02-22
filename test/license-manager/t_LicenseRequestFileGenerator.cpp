///
/// @file   t_LicenseRequestFileGenerator.cpp
/// @author Dohyun Yun
/// @date   2010-07-27
///

#include <license-manager/LicenseRequestFileGenerator.h>
#include <boost/test/unit_test.hpp>
#include <iostream>

using namespace boost::unit_test;
using namespace license_module;

BOOST_AUTO_TEST_SUITE(test_LicenseRequestFileGenerator)

BOOST_AUTO_TEST_CASE(test_createLicenseRequestFile)
{
    uint64_t productCode;
    std::string productCodeStr("sf11.1.0");
    memcpy(&productCode, productCodeStr.c_str(), sizeof(uint64_t));
    std::string requestFileName("request.dat");
    std::string cmd("rm -rf " + requestFileName);

    ::system(cmd.c_str());
    BOOST_CHECK( LicenseRequestFileGenerator::createLicenseRequestFile(productCode, requestFileName) );

    //LicenseRequestFileGenerator::print(requestFileName);
}

BOOST_AUTO_TEST_SUITE_END()

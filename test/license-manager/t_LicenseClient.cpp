///
/// @file   t_LicenseClient.cpp
/// @author Dohyun Yun
/// @date   2010-05-22
///

#include <boost/test/unit_test.hpp>
#include <license-manager/LicenseClient.h>

using namespace std;
using namespace license_module;
using namespace boost::unit_test;

BOOST_AUTO_TEST_SUITE(LicenseClient_test)

BOOST_AUTO_TEST_CASE(getLicenseCertificate_test)
{
    uint64_t productInfo = 2919293;
    std::string filePath(__FILE__);
    filePath += ".license";
    LicenseClient licenseClient(productInfo, "127.0.0.1", "27132", filePath);

    licenseClient.validateLicenseCertificate();
}

BOOST_AUTO_TEST_SUITE_END()

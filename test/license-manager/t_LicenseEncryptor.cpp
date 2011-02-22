///
/// @file   t_LicenseEncryptor.cpp
/// @author Dohyun Yun
/// @date   2010-05-22
///

#include <boost/test/unit_test.hpp>
#include <license-manager/LicenseEncryptor.h>

using namespace std;
using namespace boost::unit_test;
using namespace license_module;
using namespace license_tool;

BOOST_AUTO_TEST_SUITE(test_EncryptModule)

BOOST_AUTO_TEST_CASE(basic_encode_decode)
{
    size_t sysInfoSize;
    LICENSE_DATA_T sysInfo;
    license_tool::getSystemInfo(sysInfoSize, sysInfo);

    size_t inputSize = sysInfoSize;
    LICENSE_DATA_T inputData(new unsigned char[inputSize]);
    memcpy(inputData.get(), sysInfo.get(), sysInfoSize);

    // Encryption
    size_t outputSize;
    LICENSE_DATA_T outputData;
    LicenseEncryptor licenseEncryptor;
    BOOST_CHECK_EQUAL(true, licenseEncryptor.encryptData(inputSize, inputData, outputSize, outputData) );

    // Decryption
    size_t dataSize;
    LICENSE_DATA_T data;
    BOOST_CHECK_EQUAL(true, licenseEncryptor.decryptData(outputSize, outputData, dataSize, data) );

    BOOST_CHECK_EQUAL(0, memcmp( inputData.get(), data.get(), inputSize ));
} // end - basic_encode_decode

BOOST_AUTO_TEST_SUITE_END()

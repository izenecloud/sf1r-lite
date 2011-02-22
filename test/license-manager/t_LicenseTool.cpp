///
/// @file   t_LicenseTool.cpp
/// @author Dohyun Yun
/// @date   2010-05-22
///

#include <boost/test/unit_test.hpp>
#include <license-manager/LicenseTool.h>

using namespace std;
using namespace boost::unit_test;
using namespace license_module;
using namespace license_tool;

BOOST_AUTO_TEST_SUITE(test_SystemInfo)

BOOST_AUTO_TEST_CASE(license_tool)
{
    size_t tmpSysDataLen = 0;
    LICENSE_DATA_T tmpSysData(new unsigned char[license_module::SYSTEM_INFO_MAX_LENGTH]);

    BOOST_CHECK( getCPUNo() > 0 );
    BOOST_CHECK( getTotalRamSize() > 0 );
    BOOST_CHECK( getMacIPAddrList(tmpSysDataLen, tmpSysData) );
    BOOST_CHECK( tmpSysDataLen > 0 );

    tmpSysDataLen = 0;
    tmpSysData.reset(new unsigned char[license_module::SYSTEM_INFO_MAX_LENGTH]);
    unsigned char tmpSrc[8] = { 0x11, 0x97, 0x75, 0x23, 0x77, 0x67, 0x88, 0x22 };
    BOOST_CHECK_EQUAL( copyCArrToBArr(8, tmpSrc, tmpSysDataLen, tmpSysData) , static_cast<size_t>(8) );
    BOOST_CHECK_EQUAL( tmpSysDataLen , static_cast<size_t>(8) );
    for(size_t i = 0; i < tmpSysDataLen; i++)
        BOOST_CHECK_EQUAL( tmpSrc[i], tmpSysData[i] );

    unsigned long ulNum = 1888233427;
    unsigned char tmpSrc2[8];
    memcpy(tmpSrc2, &ulNum, 8);
    BOOST_CHECK_EQUAL(copyULongToBArr(1888233427, tmpSysDataLen, tmpSysData), static_cast<size_t>(16) );
    BOOST_CHECK_EQUAL( tmpSysDataLen , 16 );
    for(size_t i = 8; i < tmpSysDataLen; i++)
        BOOST_CHECK_EQUAL( tmpSrc2[i-8], tmpSysData[i] );
} // end - system_info_tool

BOOST_AUTO_TEST_CASE(getSha1HashOfFile)
{
    std::string file(__FILE__); // file name of this test case.
    LICENSE_DATA_T hashData;
    license_module::license_tool::getSha1HashOfFile(file, hashData);

    char hashCStr[41];
    sha1::toHexString(hashData.get(), hashCStr);

    std::string cmd("sha1sum ");
    cmd += __FILE__;

    FILE* out = popen(cmd.c_str(), "r");
    BOOST_CHECK( out != NULL );

    char hashCStr2[41];
    fgets(hashCStr2, 41, out);
    BOOST_CHECK_EQUAL( strcmp(hashCStr , hashCStr2) , 0 );
    pclose( out );
} // end - getSha1HashOfFile

BOOST_AUTO_TEST_CASE(LZO_Compress_Decompress)
{
    std::string file(__FILE__);
    size_t srcLen = 0, comLen, decLen;
    LICENSE_DATA_T srcData, comData, decData;

    BOOST_CHECK( license_module::license_tool::getUCharArrayOfFile(file, srcLen, srcData) );

    license_module::license_tool::LZO_setDefaultCompSize( srcLen, comData );

    BOOST_CHECK( license_module::license_tool::LZO_Compress(srcData, srcLen, comData, comLen) );

    decData.reset( new unsigned char[ srcLen ] );
    BOOST_CHECK( license_module::license_tool::LZO_Decompress(comData, comLen, decData, decLen) );

    for(size_t i = 0; i < srcLen; i++)
        BOOST_CHECK_EQUAL( srcData[i] , decData[i] );
} // end - LZO_Compress_Decompress

BOOST_AUTO_TEST_CASE(SEED256_Encryption)
{
    size_t i;
    size_t dataSize, originSize = 35;
    
    boost::shared_array<BYTE> data( new BYTE[originSize] ), origin( new BYTE[originSize] );
    boost::shared_array<BYTE> userKey( new BYTE[license_module::SEED256_BLOCK_SIZE] );
    boost::shared_array<DWORD> roundKey(new DWORD[license_module::SEED256_ROUNDKEY_SIZE]);

    for(i = 0; i < originSize; i++)
        data[i] = rand() % 0xff;
    for(i = 0; i < license_module::SEED256_BLOCK_SIZE; i++)
        userKey[i] = rand() % 0xff;

    // copy data to origin for decompression data comparison.
    memcpy( origin.get(), data.get(), originSize );

    // Generate Round Key (Private Key) which is used encryption.
    SeedRoundKey( roundKey.get(), userKey.get() );

    // SEED256_encode() returns false because dataSize is not suitable to be divided by 256 bits.
    BOOST_CHECK_EQUAL( false , license_module::license_tool::SEED256_encode( roundKey, originSize, data ) );

    // SEED256_padding() returns true because dataSize is changed.
    BOOST_CHECK_EQUAL( true , license_module::license_tool::SEED256_padding( originSize, dataSize, data, true ) );

    // SEED256_encode() returns true because dataSize is suitable to apply SEED256 encryption.
    BOOST_CHECK_EQUAL( true, license_module::license_tool::SEED256_encode( roundKey, dataSize, data ) );

    license_module::license_tool::SEED256_decode( roundKey, dataSize, data );

    for(i = 0; i < originSize;i++)
        BOOST_CHECK_EQUAL( origin[i] , data[i] );

} // end - SEED256_Encryption

BOOST_AUTO_TEST_CASE(getXXXDate)
{
    uint32_t currentDate = license_module::license_tool::getCurrentDate();
    BOOST_CHECK_EQUAL( 20100910 , license_module::license_tool::getStartDate(currentDate) - currentDate );
    BOOST_CHECK_EQUAL( 20100820 , license_module::license_tool::getEndDate(currentDate) - currentDate );
} // end - getLocalUTCTime

BOOST_AUTO_TEST_CASE(cvtArrToStr_cvtStrToArr)
{
    LICENSE_DATA_T srcArr(new unsigned char[5]), cmpArr;
    size_t cmpSize;
    std::string arrStr;
    srcArr[0] = 0x12; srcArr[1] = 0xFE; srcArr[2] = 0x00; srcArr[3] = 0x34; srcArr[4] = 0x12;

    BOOST_CHECK( cvtArrToStr(5, srcArr, arrStr) );
    BOOST_CHECK_EQUAL("12FE003412" , arrStr);

    BOOST_CHECK( cvtStrToArr(arrStr, cmpSize, cmpArr) );
    BOOST_CHECK_EQUAL( 5 , cmpSize );
    for(size_t i = 0;  i < cmpSize; i++)
        BOOST_CHECK_EQUAL( srcArr[i] , cmpArr[i] );
} // end - cvtArrToStr_cvtStrToArr

BOOST_AUTO_TEST_CASE(dirSize)
{
    uintmax_t t1, t2;
    std::string path("./");

    // get size by using df
    std::string cmd("du -b -s ");
    cmd += path;
    FILE* out = popen(cmd.c_str(), "r");
    BOOST_CHECK( out != NULL );

    fscanf(out, " %ju ", &t1);
    pclose(out);
    t2 = license_module::license_tool::dirSize(path, true);

    double diff = t1 - t2;
    if (diff < 0 ) diff *= -1;
    double ratio = diff / t1 * 100;

    BOOST_CHECK( ratio < 2.0 );
} // end - dirSize

BOOST_AUTO_TEST_SUITE_END()

///
/// @file   LicenseClient.cpp
/// @brief  A source file of license client.
/// @author Dohyun Yun ( dualistmage@gmail.com )
/// @date   2010-05-18
///

#include "LicenseClient.h"
#include "LicenseTool.h"

#include <boost/filesystem.hpp>

LICENSE_NAMESPACE_BEGIN

LicenseClient::LicenseClient(
        uint64_t productInfo, 
        const std::string& licenseServerIP,
        const std::string& licenseServerPort,
        const std::string& licenseFilePath,
        bool systemInfoType):
    productInfo_(productInfo),
    licenseFilePath_(licenseFilePath),
    systemInfoType_(systemInfoType)
{
    license_tool::getSystemInfo(systemInfoSize_, systemInfoData_, systemInfoType_);
} // end - LicenseClient()

bool LicenseClient::validateLicenseCertificate()
{
#ifndef COBRA_RESTRICT
    return true;
#endif

#if LICENSE_MAX_DOC == 10000 // If it is cobra
    return true;
#else
    if ( !validateLicenseFile(licenseFilePath_) )
        return false;

    if ( validateNetworkLicense() )
        return true;

    return true;
#endif // #if LICENSE_MAX_DOC == 10000
} // end - validateLicenseCertificate()

bool LicenseClient::validateLicenseFile(const std::string& licenseFile)
{
#ifndef COBRA_RESTRICT
    return true;
#endif

#if LICENSE_MAX_DOC == 10000 // If it is cobra
    std::cout << "This is Cobra Version" << std::endl;
    return true;
#else
    //If License file doesn't exist, return false;
    if ( !boost::filesystem::exists(licenseFile) )
        return false;

    size_t fileSize, rawSize, offset;
    LICENSE_DATA_T fileData, rawData;

    if ( !license_tool::getUCharArrayOfFile( licenseFile, fileSize, fileData ) )
        return false;

    if ( !licenseEncryptor_.decryptData(fileSize, fileData, rawSize, rawData) )
        return false;

    // If product code in license code is not matched
    if ( memcmp(&productInfo_, rawData.get(), PRODUCT_CODE_SIZE) != 0 )
        return false;

    // If system information is different
    offset = license_module::PRODUCT_CODE_SIZE;
    sysinfo_size_t sysInfoSize;
    memcpy(&sysInfoSize, rawData.get() + offset, sizeof(sysinfo_size_t));
    offset += sizeof(sysinfo_size_t);

    size_t sysInfoTypeOffset = offset + sysInfoSize - 1;
    systemInfoType_  = rawData[sysInfoTypeOffset];
    license_tool::getSystemInfo(systemInfoSize_, systemInfoData_, systemInfoType_);

    if ( sysInfoSize != systemInfoSize_ || memcmp(systemInfoData_.get(), rawData.get() + offset, sysInfoSize) != 0 )
        return false;

    // If current time is expired
    offset += sysInfoSize;
    uint32_t licenseDate[2];
    memcpy(licenseDate, rawData.get() + offset, LICENSE_DATE_SIZE); // DATE : Start + End
    uint32_t currentDate = license_tool::getCurrentDate();

    if ( licenseDate[LICENSE_DATE_START] > license_tool::getStartDate(currentDate) || 
            licenseDate[LICENSE_DATE_END] < license_tool::getEndDate(currentDate) )
    {
        std::cout << "<<<<< License Date is expired. Please renew your license file >>>>>" << std::endl;
        return false;
    }
    offset += LICENSE_DATE_SIZE;

    // If document limitation is not matched
    license_max_doc_t maxIndexNum;
    license_max_index_data_size_t maxIndexSize;
    memcpy(&maxIndexNum, rawData.get() + offset, sizeof(license_max_doc_t)); 
    offset += sizeof(license_max_doc_t);
    memcpy(&maxIndexSize, rawData.get() + offset, sizeof(license_max_index_data_size_t)); 
    offset += sizeof(license_max_doc_t);

    if ( maxIndexNum < LICENSE_MAX_DOC )
        return false;
    if ( maxIndexSize < LICENSE_MAX_SIZE )
        return false;

    return true;
#endif // #if LICENSE_MAX_DOC == 10000
} // end - validateLicenseFile()

int LicenseClient::validateNetworkLicense()
{
    /*
    size_t certSize; LICENSE_DATA_T certData;
    
    // Extracts certificate data from license file.
    if ( !extractCertificateDataFromLicenseFile( licenseFilePath_, certSize, certData ) )
        return LICENSE_NETWORK_VALIDATION_FAILED;

    // Prepare request data to get network certificate data from LicenseServer.
    size_t reqSize = REQ_HEADER_SIZE + DATE_SIZE*2 + systemInfoSize_, netCertSize;
    size_t curLocate = 0;
    LICENSE_DATA_T reqData(new unsigned char[reqSize]), netCertData;
    reqData[0] = REQ_CERTIFICATE; 
    curLocate += REQ_HEADER_SIZE;
    memcpy(reqData.get() + curLocate , &productInfo_, DATE_SIZE); 
    curLocate += DATE_SIZE;
    memcpy(reqData.get() + curLocate , systemInfoData_.get(), systemInfoSize_);

    // Send request data to LicenseServer
    if ( !licenseNetClient_.request( reqSize, reqData, netCertSize, netCertData ) )
        return LICENSE_NETWORK_VALIDATION_FAILED;

    // Check systeminfo from certificate data.
    if ( memcmp(netCertData.get(), systemInfoData_.get(), systemInfoSize_) != 0 )
        return LICENSE_NETWORK_VALIDATION_FAILED;

    //certSize = netCertSize;
    //certData.swap( netCertData );

    */
    return LICENSE_NETWORK_VALIDATION_SUCCESS;
} // end - validateLicenseFile()

bool LicenseClient::extractCertificateDataFromLicenseFile(
        const std::string& licenseFile,
        size_t& certSize,
        LICENSE_DATA_T& certData)
{
    // TODO : Dohyun Yun
    return true;
} // end - extractCertificateDataFromLicenseFile()

LICENSE_NAMESPACE_END

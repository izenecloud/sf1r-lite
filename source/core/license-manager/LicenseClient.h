///
/// @file   LicenseClient.h
/// @brief  A header file of license client.
/// @author Dohyun Yun ( dualistmage@gmail.com )
/// @date   2010-05-18
/// @date
///

#ifndef _LICENSE_CLIENT_H_
#define _LICENSE_CLIENT_H_

#include "LicenseEnums.h"
#include "LicenseEncryptor.h"

LICENSE_NAMESPACE_BEGIN

class LicenseClient
{
    public:
        ///
        /// @brief defualt constructor.
        /// @param[productInfo] Product information value. (2 Bytes Product Type + 6 Bytes Product version)
        /// @param[licenseServerIP] IP address of license server.
        /// @param[licenseServerPort] Port number of license server.
        /// @param[licenseFilePath] path of license file.
        /// @param[systemInfoType] type of system info. true will include UUID.
        ///
        LicenseClient(
                uint64_t productInfo, 
                const std::string& licenseServerIP,
                const std::string& licenseServerPort,
                const std::string& licenseFilePath,
                bool systemInfoType = false);

        ///
        /// @brief This interface gets license certificates by using license file and network check.
        /// @details 
        ///     - validateLicenseFile() and validateNetworkLicense() interfaces are called by this interface.
        /// @return true if product is in the valid license, or false.
        ///
        bool validateLicenseCertificate();

        ///
        /// @brief This interface validates license file.
        /// @param[
        /// @return true if product is in the valid file license, or false.
        ///
        bool validateLicenseFile(const std::string& licenseFile);

        ///
        /// @brief This interface validates license file.
        /// @return 1   means success in network license varification.
        /// @return 0   means network license is not required.
        /// @return -1  means fail to network license varification.
        ///

        int validateNetworkLicense();

    private:

        ///
        /// @brief This function extracts license data from license file.
        /// @param[licenseFile] file name of license data source.
        /// @param[certDataSize] Data size of license certification from license file.
        /// @param[certData] license certification data in license file.
        /// @return true if success, or false.
        ///
        bool extractCertificateDataFromLicenseFile(
                const std::string& licenseFile, 
                size_t& certSize,
                LICENSE_DATA_T& certData);

        ///
        /// @brief Product information value. (2 Bytes Product Type + 6 Bytes Product version)
        ///
        uint64_t productInfo_;

        ///
        /// @brief a string which contains path of license file.
        ///
        std::string licenseFilePath_;

        ///
        /// @brief an object which manages encryption in license client.
        ///
        LicenseEncryptor licenseEncryptor_;

        ///
        /// @brief size of systemInfo data
        ///
        size_t systemInfoSize_;

        ///
        /// @brief an system info data of local machine.
        ///
        LICENSE_DATA_T systemInfoData_;

        ///
        /// @brief type of systemInfo. true will include UUID while false will not.
        ///
        bool systemInfoType_;

}; // end - LicenseClient

LICENSE_NAMESPACE_END
#endif // _LICENSE_CLIENT_H_

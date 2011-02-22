///
/// @file   LicenseDataManager.h
/// @brief  A header file of license data manager.
/// @author Dohyun Yun ( dualistmage@gmail.com )
/// @date
///

#ifndef _LICENSE_DATA_MANAGER_H_
#define _LICENSE_DATA_MANAGER_H_

namespace license_manager {
    class LicenseDataManager
    {
        public:
            bool insertLicenseData(
                    const std::string& key,
                    const std::string& value );

            bool deleteLicenseData(
                    const std::string& key,
                    const std::string& value );

            bool updateLicenseData(
                    const std::string& key,
                    const std::string& value );

            bool getLicenseData(
                    const std::string& key,
                    std::string& value );
    }; // end - LicenseDataManager
} // end - namespace license_manager

#endif // _LICENSE_DATA_MANAGER_H_

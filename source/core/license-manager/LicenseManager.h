///
/// @file   LicenseManager.h
/// @brief  A header file of license manager.
/// @author Dohyun Yun ( dualistmage@gmail.com )
/// @date   2010-05-18
///

#ifndef _LICENSE_MANAGER_H_
#define _LICENSE_MANAGER_H_

#include "LicenseClient.h"
#include "LicenseEnums.h"

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#include <vector>

namespace sf1r {

class LicenseManager
{
    public:
        enum LicenseGrade { 
            COBRA_LICENSE = 0, 
            SILVER_LICENSE, 
            GOLD_LICENSE, 
            PLATINUM_LICENSE, 
            MAX_SIZE_TEST_LICENSE, 
            MAX_DOC_SIZE_TEST_LICENSE, 
            DEV_LICENCE 
        };

    public:

        LicenseManager(const std::string& sf1Version, const std::string& licenseFilePath, bool systemInfoType = false);

        bool validateLicense();

        bool validateLicenseFile();

        bool createLicenseRequestFile(const std::string& licenseRequestFile);

        void startBGWork(std::vector<std::string> indexPathList);

        static license_module::license_concurrency_t getMaxConcurrency();

        static license_module::license_max_doc_t getMaxIndexSize();

        static license_module::license_max_index_data_size_t getMaxIndexFileSize();

        static void getLicenseLimitation(
                LicenseGrade grade, 
                license_module::license_max_doc_t& maxDocSize, 
                license_module::license_max_index_data_size_t& maxSize);

    private:

        // Background process interface.

        ///
        /// @brief background interface which checks index file size continuously during sf1-execution.
        ///     - WARNING : short interval second will degrade performance. Default is 5 hours (300 secs)
        /// @param[indexPathLIst] a list of index file directories.
        /// @param[intervalSec] interval second among index file size checking processes.
        ///
        void bgCheckIndexFileSize(const std::vector<std::string>& indexPathList, size_t intervalSec = 300);


    public:

        static bool continueIndex_;

        static const license_module::license_max_doc_t TRIAL_MAX_DOC = 10000;
        static const license_module::license_max_index_data_size_t TRIAL_MAX_SIZE = 1024 * 1024 * 1024;
        static const std::string LICENSE_REQUEST_FILENAME;
        static const std::string LICENSE_KEY_FILENAME;

    private:

        ///
        /// @brief License Client which manages license validation.
        ///
        boost::shared_ptr<license_module::LicenseClient> licenseClient_;

        uint64_t productCode_;

        std::string licenseFilePath_;

        bool systemInfoType_;

        static boost::mutex mutex_;

        static license_module::license_concurrency_t maxConcurrency_;

        static license_module::license_max_doc_t maxIndexSize_;

        static license_module::license_max_index_data_size_t maxIndexFileSize_;


        static const license_module::license_max_index_data_size_t GB = 1024 * 1024 * 1024;
        static const license_module::license_max_doc_t K = 1000;
        static const license_module::license_max_doc_t M = K * K;

}; // end - class LicenseManager

} // end - namespace sf1r

#endif // _LICENSE_MANAGER_H_

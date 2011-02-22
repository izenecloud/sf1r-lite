///
/// @file   LicenseManager.cpp
/// @brief  A source file of license manager.
/// @author Dohyun Yun ( dualistmage@gmail.com )
/// @date   2010-05-18
///

#include "LicenseManager.h"
#include "LicenseRequestFileGenerator.h"

#include <common/SFLogger.h>

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include <sstream>

using namespace sf1r;
using namespace license_module;

boost::mutex LicenseManager::mutex_;
license_module::license_max_doc_t LicenseManager::maxIndexSize_(0);
license_module::license_max_index_data_size_t LicenseManager::maxIndexFileSize_(0);
bool LicenseManager::continueIndex_(true);
const std::string LicenseManager::LICENSE_REQUEST_FILENAME = "sf1-license-request.dat";
const std::string LicenseManager::LICENSE_KEY_FILENAME = "sf1-license-key.dat";

LicenseManager::LicenseManager(const std::string& sf1Version, const std::string& licenseFilePath, bool systemInfoType):
    systemInfoType_(systemInfoType)
{
    std::string productCodeStr("SF1");
    productCodeStr += sf1Version;
    memcpy(&productCode_, productCodeStr.c_str(), sizeof(uint64_t));
    licenseFilePath_ = licenseFilePath;

    licenseClient_.reset(
            new LicenseClient(
            productCode_,
            "127.0.0.1",    // TODO
            "23456",        // TODO
            licenseFilePath_,
            systemInfoType)
            );
} // end - LicenseManager()

bool LicenseManager::validateLicense()
{
#ifdef COBRA_RESTRICT
    if ( licenseClient_ )
        return licenseClient_->validateLicenseCertificate();
    return false;
#else
    return true;
#endif // COBRA_RESTICT
} // end - validateLicense()

bool LicenseManager::validateLicenseFile()
{
#ifdef COBRA_RESTRICT
    if ( licenseClient_ )
        return licenseClient_->validateLicenseFile(licenseFilePath_);
    return false;
#else
    return true;
#endif // COBRA_RESTICT
}

bool LicenseManager::createLicenseRequestFile(const std::string& licenseRequestFile)
{
    return LicenseRequestFileGenerator::createLicenseRequestFile( productCode_, licenseRequestFile, systemInfoType_ );
} // end - createLicenseRequestFile()

void LicenseManager::startBGWork(std::vector<std::string> indexPathList)
{
    boost::thread_group bgWork;
    bgWork.create_thread(boost::bind(&LicenseManager::bgCheckIndexFileSize, this, indexPathList, 600)); // disk size check every 10 mins
    bgWork.join_all();
} // end - isIndexFileSizeValid()

license_module::license_max_doc_t LicenseManager::getMaxIndexSize()
{
    boost::mutex::scoped_lock lock(mutex_);
    return maxIndexSize_;
} // end - getMaxIndexSize()

license_module::license_max_index_data_size_t LicenseManager::getMaxIndexFileSize()
{
    boost::mutex::scoped_lock lock(mutex_);
    return maxIndexFileSize_;
} // end - getMaxIndexFileSize()

void LicenseManager::bgCheckIndexFileSize(const std::vector<std::string>& indexPathList, size_t intervalSec)
{
#ifdef COBRA_RESTRICT
    license_module::license_max_index_data_size_t totalSize;
    while(1) {
        // std::stringstream ss;
        totalSize = 0;
        std::vector<std::string>::const_iterator indexPathIter = indexPathList.begin();
        for(; indexPathIter != indexPathList.end(); indexPathIter++)
        {
            totalSize += license_tool::dirSize(*indexPathIter, true);
            // ss << "-----[ " << *indexPathIter << " : " << license_tool::dirSize(*indexPathIter, true) << std::endl;
        }
        
        // ss << "----------[ " << "TOTAL SIZE : " << totalSize << std::endl;
        // std::cerr << ss.str() << std::endl;
        if ( totalSize > LICENSE_MAX_SIZE * GB )
        {
            sflog->error(SFL_INIT, 150001, totalSize, getMaxIndexFileSize());
            continueIndex_ = false;
            //kill(getpid(), SIGTERM);
        }
        sleep(intervalSec);
    }
#else
    std::cerr << "-----[ Background Process of License checking is disabled" << std::endl;
#endif // COBRA_RESTRICT
} // end - bgCheckIndexFileSize()

void LicenseManager::getLicenseLimitation(LicenseGrade grade, license_module::license_max_doc_t& maxDocSize, license_module::license_max_index_data_size_t& maxSize)
{
    switch(grade) {
        case COBRA_LICENSE:
            maxDocSize          = 10 * K;
            maxSize             = 1 * GB;
            break;
        case SILVER_LICENSE:
            maxDocSize          = 100 * K;
            maxSize             = 10 * GB;
            break;
        case GOLD_LICENSE:
            maxDocSize          = 1 * M;
            maxSize             = 100 * GB;
            break;
        case PLATINUM_LICENSE:
            maxDocSize          = 500 * M;
            maxSize             = 50000 * GB;
            break;
        case MAX_SIZE_TEST_LICENSE:
            maxDocSize          = 500 * M;
            maxSize             = 1024 * 1024; // 1M
            break;
        case MAX_DOC_SIZE_TEST_LICENSE:
            maxDocSize          = 1 * K;
            maxSize             = 5000 * GB;
            break;
        case DEV_LICENCE:
            maxDocSize          = 500 * M;
            maxSize             = 50000 * GB;
            break;
        default:
            maxDocSize          = 0;
            maxSize             = 0;
    }
}

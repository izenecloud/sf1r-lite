///
/// @file LicenseRequestFileGenerator.h
/// @brief header file of License request file generator
/// @author Dohyun Yun
/// @date 2010-07-27
/// @log
///

#ifndef _LICENSE_REQUEST_FILE_GENERATOR_H_
#define _LICENSE_REQUEST_FILE_GENERATOR_H_

#include "LicenseEnums.h"
#include <iostream>

LICENSE_NAMESPACE_BEGIN

namespace LicenseRequestFileGenerator {

    ///
    /// @brief creates license request file
    ///
    bool createLicenseRequestFile(uint64_t productInfo, const std::string& outputFile, bool includeUUID = false);

    ///
    /// @brief print content of license request file.
    ///
    void printRequestFile(const std::string& requestFile, std::ostream& out = std::cout);

    ///
    /// @brief print content of license Data file.
    ///
    void printLicenseFile(const std::string& requestFile, std::ostream& out = std::cout);

    ///
    /// @brief print content of license Data file, which contains total use time and each user's info.
    ///
    void printLicenseFileInFo(const std::string& requestFile, std::ostream& out = std::cout);

}; // end - namespace LicenseRequestFileGenerator

LICENSE_NAMESPACE_END

#endif // _LICENSE_REQUEST_FILE_GENERATOR_H_

///
/// @file LicenseRequestFileGenerator.cpp
/// @brief source file of License request file generator
/// @author Dohyun Yun
/// @date 2010-07-27
///

#include "LicenseRequestFileGenerator.h"
#include "LicenseEncryptor.h"
#include "LicenseTool.h"
#include <fstream>
#include <sstream>

LICENSE_NAMESPACE_BEGIN

namespace LicenseRequestFileGenerator {

    bool createLicenseRequestFile(uint64_t productInfo, const std::string& outputFile, bool includeUUID)
    {
        using namespace std;

        try {

            size_t licenseRequestSize, tmpSize;
            LICENSE_DATA_T licenseRequestData, tmpData;

            // Store system info
            if ( !license_tool::getSystemInfo(tmpSize, tmpData, includeUUID) )
                return false;

            // Store productInfo
            memcpy( tmpData.get() + tmpSize, &productInfo, sizeof(uint64_t) );
            tmpSize += sizeof(uint64_t);

            // Encryption request data
            LicenseEncryptor licenseEncryptor;
            licenseEncryptor.encryptData(tmpSize, tmpData, licenseRequestSize, licenseRequestData);

            // Write into file
            std::ofstream fpout(outputFile.c_str());
            if ( !fpout.is_open() )
            {
                cerr << "createLicenseRequestFile() Error : cannot open " << outputFile << endl;
                return false;
            }
            fpout.write( (char*)(licenseRequestData.get()), licenseRequestSize );
            fpout.close();
        } catch ( exception e ) {
            cerr << "createLicenseRequestFile() Exception : " << e.what() << endl;
            return false;
        }

        return true;
    } // end - createLicenseRequestFile()

    void printRequestFile(const std::string& requestFile, std::ostream& out)
    {
        using namespace std;
        try {
            size_t tmpSize, requestSize;
            LICENSE_DATA_T tmpData, requestData;
            if ( !license_tool::getUCharArrayOfFile(requestFile, tmpSize, tmpData) )
                return;

            // Decryption
            LicenseEncryptor licenseEncryptor;
            licenseEncryptor.decryptData(tmpSize, tmpData, requestSize, requestData);

            // Extract Data
            char productCode[8];

            // Extract product code
            memcpy(productCode, requestData.get() + requestSize - PRODUCT_CODE_SIZE, PRODUCT_CODE_SIZE); 

            // Extract SysInfo
            license_tool::SystemInfo sysInfo;
            sysInfo.init(0, requestSize - PRODUCT_CODE_SIZE, requestData);

            // Print
            stringstream ss;
            ss << "LicenseRequest Report" << endl;
            ss << "---------------------" << endl;
            ss << "Data File : " << requestFile << endl;
            ss << "Data Size : " << requestSize << endl;
            ss << "ProductCD : "; for(size_t i=0; i<8; i++) ss << productCode[i]; ss << endl;
            sysInfo.print(ss);
            out << ss.str();
        } catch ( exception e ) {
            cerr << "print() Exception : " << e.what() << endl;
        }

    } // end - printRequestFile()

    void printLicenseFile(const std::string& licenseFile, std::ostream& out)
    {
        using namespace std;
        try {
            size_t tmpSize, licenseSize, index;
            LICENSE_DATA_T tmpData, requestData;
            if ( !license_tool::getUCharArrayOfFile(licenseFile, tmpSize, tmpData) )
                return;

            // Decryption
            LicenseEncryptor licenseEncryptor;
            licenseEncryptor.decryptData(tmpSize, tmpData, licenseSize, requestData);

            // Extract Data
            char productCode[8];

            // Extract product code
            memcpy(productCode, requestData.get(), license_module::PRODUCT_CODE_SIZE ); index = license_module::PRODUCT_CODE_SIZE;

            // Extract Sysinfo 
            license_tool::SystemInfo sysInfo;
            sysinfo_size_t sysInfoSize;
            memcpy(&sysInfoSize, requestData.get() + index, sizeof(sysinfo_size_t)); index += sizeof(sysinfo_size_t);
            sysInfo.init(index, sysInfoSize, requestData);
            index += sysInfoSize;

            // Extract Start & End Date
            uint32_t licenseDate[2];
            memcpy(&licenseDate, requestData.get() + index, sizeof(uint32_t) * 2); index += sizeof(uint32_t) * 2;
            licenseDate[0] -= license_module::encValueOfStartDate;
            licenseDate[1] -= license_module::encValueOfEndDate;

            // Extract License Level
            license_max_doc_t maxIndexNum;
            license_module::license_max_index_data_size_t maxIndexSize;
            memcpy(&maxIndexNum, requestData.get() + index, sizeof(license_max_doc_t)); 
            index += sizeof(license_module::license_max_doc_t);
            memcpy(&maxIndexSize, requestData.get() + index, sizeof(license_max_index_data_size_t));

            // Print
            stringstream ss;
            ss << "License Data" << endl;
            ss << "---------------------" << endl;
            ss << "Data File    : " << licenseFile << endl;
            ss << "Data Size    : " << licenseSize << endl;
            ss << "ProductCD    : "; for(size_t i=0; i<8; i++) ss << productCode[i]; ss << endl;
            ss << "SYSInfo Size : " << sysInfoSize << endl;
            sysInfo.print(ss);
            ss << "License Date : " << licenseDate[0] << " ~ " << licenseDate[1] << endl;
            ss << "MaxDocSize   : " << maxIndexNum  << endl;
            ss << "MaxIndexSize : " << maxIndexSize << endl;

            out << ss.str();
        } catch ( exception e ) {
            cerr << "print() Exception : " << e.what() << endl;
        }

    } // end - printLicenseFile()

} // end - namespace LicenseRequestFileGenerator

LICENSE_NAMESPACE_END


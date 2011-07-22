#include <license-manager/LicenseManager.h>
#include <license-manager/LicenseRequestFileGenerator.h>
#include <util/sysinfo/dmiparser.h>

#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;
using namespace sf1r;
using namespace license_module;

const int CREATE_REQ_DATA = 1;
const int CREATE_LICENSE_DATA = 2;

int  displayMenu();
void createLicenseFile();
void createLocalReqFile();
void getValueOrExit(const std::string& message, std::string& output);
void getLicenseFilename(const std::string& message, std::string& output, bool isRequestFile=true);
bool getLicenseLevel(
        license_max_doc_t& maxIndexNum, 
        license_max_index_data_size_t& maxIndexSize);
bool extractProductCodeNSysInfo( 
        uint64_t& productCode, 
        uint32_t& sysInfoSize, LICENSE_DATA_T& sysInfoData);
bool extractStartNEndDate(uint32_t& startDate, uint32_t& endDate);

int main()
{
    while(1) {
        switch ( displayMenu() ) {
            case CREATE_REQ_DATA:
                createLocalReqFile();
                break;
            case CREATE_LICENSE_DATA:
                createLicenseFile();
                break;
            default:
                continue;
        }
    }
}

int displayMenu()
{
    stringstream ss;
    ss << "Select Menu" << endl;
    ss << "------------------------------" << endl;
    ss << "1. Create License Request File" << endl;
    ss << "2. Create License File" << endl;
    ss << "------------------------------" << endl;
    ss << "Select : ";
    cout << ss.str();
    int input;
    cin >> input;
    return input;
} // end - displayMenu()

void createLicenseFile()
{
    uint32_t sysInfoSize;
    uint32_t licenseDate[2];
    uint64_t productCode;
    license_max_doc_t maxIndexNum;
    license_max_index_data_size_t maxIndexSize;

    std::string tmp, reqFileName, licenseFileName;
    LICENSE_DATA_T sysInfoData;

    // Get data from license request file
    while(!extractProductCodeNSysInfo(productCode, sysInfoSize, sysInfoData));

    // Get Start & End date of license
    while(!extractStartNEndDate(licenseDate[0], licenseDate[1]));

    // Get License Level
    while(!getLicenseLevel(maxIndexNum, maxIndexSize));

    // Prepare raw license data
    size_t licenseSize = 0;
    LICENSE_DATA_T licenseData(new unsigned char[LICENSE_INFO_MAX_LENGTH]);
    memcpy(licenseData.get(), &productCode, PRODUCT_CODE_SIZE); licenseSize += PRODUCT_CODE_SIZE;
    memcpy(licenseData.get() + licenseSize, &sysInfoSize, license_module::SYSINFOSIZE_SIZE); licenseSize += license_module::SYSINFOSIZE_SIZE;
    memcpy(licenseData.get() + licenseSize, sysInfoData.get(), sysInfoSize); licenseSize += sysInfoSize;
    memcpy(licenseData.get() + licenseSize, licenseDate, LICENSE_DATE_SIZE); licenseSize += LICENSE_DATE_SIZE;
    memcpy(licenseData.get() + licenseSize, &maxIndexNum, sizeof(license_max_doc_t)); 
    licenseSize += sizeof(license_max_doc_t);
    memcpy(licenseData.get() + licenseSize, &maxIndexSize, sizeof(license_max_index_data_size_t)); 
    licenseSize += sizeof(license_max_index_data_size_t);

    // { // DEBUG
    //     std::string licenseDataStr ;
    //     license_tool::cvtArrToStr( licenseSize, licenseData, licenseDataStr);
    //     std::cerr << "License Data String in LFG() : " << licenseDataStr << std::endl;
    // } // DEBUG

    // Encrypt license data
    size_t encSize;
    LICENSE_DATA_T encData;
    LicenseEncryptor licenseEncryptor;
    licenseEncryptor.encryptData(licenseSize, licenseData, encSize, encData);

    // Write data into file
    getLicenseFilename("Enter license file name", licenseFileName, false);
    ofstream fpout(licenseFileName.c_str());
    fpout.write( (char*)(encData.get()), encSize );
    fpout.close();
    LicenseRequestFileGenerator::printLicenseFile(licenseFileName);

} // end - createLicenseFile()

void createLocalReqFile()
{
    uint64_t productCode;
    string tmp;
    getValueOrExit("Product Code (8 Characters)", tmp);
    memcpy(&productCode, tmp.c_str(), sizeof(uint64_t) );
    getValueOrExit("License Request file name", tmp);
    LicenseRequestFileGenerator::createLicenseRequestFile(productCode, tmp);
    LicenseRequestFileGenerator::printRequestFile(tmp);
} // end - createLocalReqFile()

void getValueOrExit(const std::string& message, std::string& output)
{
    std::string input;
    cout << message << " (q for exit) : "; cin >> input;
    if ( input == "q" ) 
        exit(-1);
    output.swap( input );
} // end - getValue()

void getLicenseFilename(const std::string& message, std::string& output, bool isRequestFile)
{
    std::string input;
    std::string fileName = isRequestFile?LicenseManager::LICENSE_REQUEST_FILENAME:LicenseManager::LICENSE_KEY_FILENAME;
    cout << message << " (q for exit, d for using default path : $HOME/sf1-license/" << fileName << ") : ";
    cin >> input;

    if ( input == "q" ) 
        exit(-1);
    else if ( input == "d" )
    {
        char* home = getenv("HOME");
        input = home;
        input += "/sf1-license/";
        input += fileName;
    }
    output.swap( input );
} // end - getValue()

bool getLicenseLevel(
        license_max_doc_t& maxIndexNum, 
        license_max_index_data_size_t& maxIndexSize)
{
    int licenseLevel;
    cout << "License Level (0:COBRA, 1:SILVER, 2:GOLD, 3:PLATINUM, 4:MAX_SIZE_TEST, 5:MAX_DOC_SIZE_TEST, 6:DEV_LICENSE) : "; cin >> licenseLevel;

    license_max_doc_t tmpMaxIndexNum;
    license_max_index_data_size_t tmpMaxIndexSize;
    LicenseManager::getLicenseLimitation(static_cast<LicenseManager::LicenseGrade>(licenseLevel), tmpMaxIndexNum, tmpMaxIndexSize);

    if ( tmpMaxIndexNum == 0 || tmpMaxIndexSize == 0 )
        return false;

    maxIndexNum = tmpMaxIndexNum;
    maxIndexSize = tmpMaxIndexSize;
    return true;
} // end - getLicenseLevel()

bool extractProductCodeNSysInfo(
        uint64_t& productCode, 
        uint32_t& sysInfoSize,
        LICENSE_DATA_T& sysInfoData)
{
    string reqFile;
    size_t tmpSize, reqSize;
    LICENSE_DATA_T tmpData, reqData;

    getLicenseFilename("Enter license request file name", reqFile);
    if ( !license_tool::getUCharArrayOfFile(reqFile, tmpSize, tmpData) ) {
        cout << "Read Error : " << reqFile << endl;
        return false;
    }

    LicenseRequestFileGenerator::printRequestFile(reqFile);

    LicenseEncryptor licenseEncryptor;
    licenseEncryptor.decryptData(tmpSize, tmpData, reqSize, reqData);

    // { // DEBUG
    //     std::string requestStr;
    //     license_tool::cvtArrToStr( reqSize, reqData, requestStr);
    //     std::cerr << "Request String in LFG() : " << requestStr << std::endl;
    // } // DEBUG

    // Extract productCode
    sysInfoSize = reqSize - PRODUCT_CODE_SIZE;
    memcpy(&productCode, reqData.get() + sysInfoSize, PRODUCT_CODE_SIZE );

    // Extract sysinfo Code
    sysInfoData.reset( new unsigned char[sysInfoSize] );
    memcpy(sysInfoData.get(), reqData.get(), sysInfoSize);

    return true;
} // end - bool extractProductCodeNSysInfo(

bool extractStartNEndDate(uint32_t& startDate, uint32_t& endDate)
{
    uint32_t tmpStartDate, tmpEndDate;
    uint32_t currentDate, date;
    currentDate = license_tool::getCurrentDate();
    cout << "Start Date (Format:YYYYMMDD) : "; cin >> date;
    tmpStartDate = license_tool::getStartDate( date );
    cout << "-- Recognized : " << date << " ==> " << tmpStartDate << endl;

    cout << "End   Date (Format:YYYYMMDD) : "; cin >> date;
    tmpEndDate = license_tool::getEndDate( date );
    cout << "-- Recognized : " << date << " ==> " << tmpEndDate << endl;

    startDate = tmpStartDate;
    endDate = tmpEndDate;
    return true;
} // end - extractStartNEndDate()

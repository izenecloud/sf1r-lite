///
/// @file LicenseTools.h
/// @brief header file of License Tools
/// @author Dohyun Yun
/// @date 2010-05-07
/// @log
///

#ifndef _LICENSE_TOOLS_H_
#define _LICENSE_TOOLS_H_

#include "LicenseEnums.h"
#include "encryption/SEED_256_KISA.h"
#include "hash/sha1.h"

#include <compression/minilzo/minilzo.h>

#include <boost/shared_array.hpp>
#include <iostream>
#include <sstream>

LICENSE_NAMESPACE_BEGIN

namespace license_tool {

    ///
    /// @brief returns the number of cpu in local machine.
    /// @return the number of cpu.
    ///
    int getCPUNo(void);

    ///
    /// @brief returns total size of ram.
    /// @return the size of total ram.
    ///
    unsigned long getTotalRamSize(void);

    ///
    /// @brief extracts mac & ip address list of local machine to the given bArrData.
    ///     NOTICE : This  interface supposes the maximum number of ethernets as 20.
    /// @param[bArrSize] the reference of size of boost::shared_array.
    /// @param[bArrData] the data area of boost::shared_array.
    /// @return true if success, or false.
    ///
    bool getMacIPAddrList(size_t& bArrSize, LICENSE_DATA_T& bArrData);

    ///
    /// @brief extracts UUID of local machine to the given bArrData
    /// @details To escape direct character recording of UUID string into bArrData, all codes are encoded.
    ///     - ASCII Code Area of UUID : 32 ~ 126
    ///     - Encoding value : 96
    ///     - Result Code of UUID : 128 ~ 222
    /// @param[bArrSize] the size of bArrData. This will be changed as much as the length of UUID.
    /// @param[bArrData] the data area of boost::shared_array.
    /// @return true if UUID is gotton, or false.
    ///
    bool getUUID(size_t&bArrSize, LICENSE_DATA_T& bArrData);

    ///
    /// @brief generates system info by using cpu, total ram, and a list of mac & ip address.
    /// @param[sysInfoSize] is an size of sysInfo data.
    /// @param[sysInfo] is an output of this interface to contain machine identical string.
    /// @return true if success, or false.
    ///
    bool getSystemInfo(size_t& sysInfoSize, LICENSE_DATA_T& sysInfo, bool includeUUID=false);

    ///
    /// @brief copy c-style array to the tail of boost shared_array.
    /// @param[cArrSize] the size of c-style array.
    /// @param[cArrData] the data area of c-style array.
    /// @param[bArrSize] the reference of size of boost::shared_array.
    /// @param[bArrData] the data area of boost::shared_array.
    /// @return updated bArrSize. bArrSize = prev_bArrSize + cArrSize.
    ///
    size_t copyCArrToBArr(
            size_t  cArrSize, unsigned char* cArrData, 
            size_t& bArrSize, LICENSE_DATA_T& bArrData);

    ///
    /// @brief copy unsigned long to the tail of boost shared_array<unsigned char>.
    /// @param[uLongData] input value of unsigned long type.
    /// @param[bArrSize] the reference of size of boost::shared_array.
    /// @param[bArrData] the data area of boost::shared_array.
    /// @return updated bArrSize.
    ///
    size_t copyULongToBArr(unsigned long uLongData, size_t& bArrSize, LICENSE_DATA_T& bArrData);

    ///
    /// @brief Stores uchar value of given file.
    /// @param[filePath] a string which contains target file path.
    /// @param[bArrSize]  size of output data array.
    /// @param[bArrData] an output of this interface which contains unsigned char data.
    /// @return true if success, or false.
    ///
    bool getUCharArrayOfFile(const std::string& filePath, size_t& bArrSize, LICENSE_DATA_T& bArrData);

    ///
    /// @brief Stores uchar value to the file.
    /// @param[filePath] a string which contains given uchar value
    /// @param[bArrSize]  size of input data array.
    /// @param[bArrData] an input data which is going to be written into file.
    /// @return true if success, or false.
    ///
    bool setUCharArrayToFile(const std::string& filePath, size_t& bArrSize, LICENSE_DATA_T& bArrData);

    ///
    /// @brief Creates sha1 hash value of given file.
    /// @param[filePath] a string which contains target file path.
    /// @param[sha1Value] an output of this interface which contains sha1 value of given file.
    /// @return true if success, or false.
    ///
    bool getSha1HashOfFile(const std::string& filePath, LICENSE_DATA_T& sha1Value);

    ///
    /// @brief LZO-Compresses data.
    /// - NOTICE : It is needed to allocate tar before using this interface.
    /// @param[src] a Source data array.
    /// @param[srcLen] length of source data.
    /// @param[tar] an output of compressed data.
    /// @param[tarLen] length of target data.
    /// @return true if success, or false.
    ///
    bool LZO_Compress(const LICENSE_DATA_T& src, size_t srcLen, 
            LICENSE_DATA_T& tar, size_t& tarLen);

    ///
    /// @brief LZO-Decompresses data.
    /// @details
    /// - NOTICE : We can't imagine the size of uncompressed data by looking at srcLen.
    ///            So it is needed to allocate tar before using this interface. 
    ///            Recommend to do recursive-decompress per fixed block if data size is big(10MB).
    /// @param[src] a Source data array.
    /// @param[srcLen] length of source data.
    /// @param[tar] an output of decompressed data.
    /// @param[tarLen] length of target data.
    /// @return true if success, or false.
    ///
    bool LZO_Decompress(const LICENSE_DATA_T& src, size_t srcLen, 
            LICENSE_DATA_T& tar, size_t& tarLen);

    ///
    /// @brief resizes tar array to the maximum size while compressing according to the length of src data.
    /// @details
    /// - All lzo libraries needed tar array to be assigned before using it.
    /// @param[srcLen] length of source data.
    /// @param[tar] an array which is going to be resized.
    ///
    void LZO_setDefaultCompSize(size_t srcLen, LICENSE_DATA_T& tar);

    ///
    /// @brief This interface apply padding specific value(0) to the encodedData so that the size could be 
    ///        divided by 256Bit BLOCK. 
    ///        (ex) Before size : 230 byte -> After Size : 256 (bit-padding 0, 26 times)
    ///        (ex) Before size : 510 byte -> After Size : 512 (bit-padding 0, 2 times)
    /// @param[srcDataSize] The size of encoded Data.
    /// @param[newDataSize] The new data size after padding.
    /// @param[encodedData] Target Data array of padding.
    /// @param[needResize] It reallocate encodedData if srcDataSize < newDataSize when the option is true.
    /// @return true if padding is happened, or false.
    ///
    bool SEED256_padding(size_t srcDataSize, size_t& newDataSize, boost::shared_array<BYTE>& encodedData, bool needResize=false);

    ///
    /// @brief SEED256-encodes the src array to tar. The length of tar is same as the one of src.
    /// @param[roundKey] a user-defined 256BIT data array called roundkey which is an symmetrical encryption key.
    /// @param[dataSize] byte size of data.
    /// @param[encodedData] an input data which is used to encrypt. Encryption is applied directly to that array.
    /// @return true if dataLen is not changed, or false.
    ///
    bool SEED256_encode(const boost::shared_array<DWORD>&roundKey, size_t dataSize, boost::shared_array<BYTE>& encodedData);

    ///
    /// @brief SEED256-decodes the src array to tar. The length of tar is same as the one of src.
    /// @param[roundKey] a user-defined 256BIT data array called roundkey which is an symmetrical encryption key. 
    ///                  This value should be same as the key which is used for encryption.
    /// @param[dataSize] byte length of data.
    /// @param[decodedData] an input data which is used to decrypt. Decryption is applied directly to that array.
    /// @return true if success, or false.
    ///
    bool SEED256_decode(const boost::shared_array<DWORD>&roundKey, size_t dataSize, boost::shared_array<BYTE>& decodedData);

    ///
    /// @brief get current date number. Format is YYYYDDMM
    /// @return current date
    ///
    uint32_t getCurrentDate();

    ///
    /// @brief get simple secure start date number. Date == YYYYDDMM + KEY(19820910)
    /// @param[date] source date which format is YYYYDDMM
    /// @return start date
    ///
    uint32_t getStartDate(uint32_t date);

    ///
    /// @brief get simple secure end date number. Date == YYYYDDMM + KEY(19820820)
    /// @param[date] source date which format is YYYYDDMM
    /// @return end date
    ///
    uint32_t getEndDate(uint32_t date);

    ///
    /// @brief shuflle data according to the utcTime.
    /// @param[utcTime] utc time which is used to shuffle data.
    /// @param[dataSize] the size of data.
    /// @param[data] data array which is going to be shuffled.
    ///
    void shuffleData(const tm& utcTime, size_t dataSize, LICENSE_DATA_T& data);

    ///
    /// @brief recover data according to the utcTime.
    /// @param[utcTime] utc time which is used to unshuffle data.
    /// @param[dataSize] the size of data.
    /// @param[data] data array which is going to be recovered.
    ///
    void unshuffleData(const tm& utcTime, size_t dataSize, LICENSE_DATA_T& data);

    ///
    /// @brief converts data array to string.
    /// @param[inputSize] size of input array.
    /// @param[inputData] array of input data.
    /// @param[outputStr] an output string which contains data array.
    /// @return true if success, or false.
    ///
    bool cvtArrToStr(size_t inputSize, const LICENSE_DATA_T& inputData, std::string& outputStr);

    ///
    /// @brief converts data string to data array.
    /// @param[inputStr] an input string which contains binary data.
    /// @param[outputSize] output size of data array.
    /// @param[outputData] array of output.
    /// @return true if success, or false.
    ///
    bool cvtStrToArr(const std::string& inputStr, size_t& outputSize, LICENSE_DATA_T& outputData);

    ///
    /// @brief calculates size of directory
    /// @param[dirpath] The path of target directory to calculate size.
    /// @param[recursive] recursive flag option. It will calculate size of sub directories if this option is on.
    /// @return the size of directory.
    ///
    uintmax_t dirSize(const std::string& dirPath, bool recursive = false);


    // For DEBUG
    void printLicenseData(const std::string& title, size_t licenseSize, const LICENSE_DATA_T& licenseData, std::ostream& out=std::cerr);





    // License Tool Classes
    ///////////////////////////////////////////////////////////////////////////

    ///
    /// @brief a class which can help to extract system information.
    ///
    class SystemInfo {
        public:
            SystemInfo() : cpuNo_(0), memorySize_(0), sysInfoType_(false) {}
            bool init(size_t sysInfoOffset, size_t sysInfoSize, LICENSE_DATA_T& bArrData)
            {
                size_t offset = sysInfoOffset;

                // Extract sysinfo type
                sysInfoType_ = bArrData[offset + sysInfoSize - 1];

                // Extract CPUNo
                cpuNo_ = static_cast<unsigned int>(bArrData[offset++]);

                // Extract memory size
                memcpy(&memorySize_, bArrData.get() + offset, sizeof(license_memory_size_t));
                offset += sizeof(license_memory_size_t);

                // Extract UUID
                if ( sysInfoType_ )
                {
                    LICENSE_DATA_UNIT_T encodedUUID[UUID_SIZE];
                    memcpy(&encodedUUID, bArrData.get() + offset, UUID_SIZE);
                    offset += UUID_SIZE;
                    for(size_t i = 0; i < UUID_SIZE; i++)
                        encodedUUID[i] -= UUID_ENCODE_VALUE;
                    UUID_.assign( reinterpret_cast<char*>(encodedUUID), UUID_SIZE );
                }

                // Extract mac address
                // -------------------
                // mac size  = sysInfoSize - (cpu + memory + UUID) - (sysInfoType)
                size_t macAddressSize = sysInfoSize - (offset - sysInfoOffset) - 1; 
                LICENSE_DATA_T macAddressArr( new LICENSE_DATA_UNIT_T[macAddressSize] );
                memcpy(macAddressArr.get(), bArrData.get() + offset, macAddressSize);
                cvtArrToStr( macAddressSize, macAddressArr, macAddressStr_ );
                return true;
            }
            unsigned int getCPUNo() { return cpuNo_; }
            license_memory_size_t getMemorySize() { return memorySize_; }
            std::string getUUIDStr() { return UUID_; }
            std::string getMacAddressStr() { return macAddressStr_; }
            bool getSystemInfoType() { return sysInfoType_; }
            void print(std::ostream& out = std::cout)
            {
                using namespace std;
                stringstream ss;
                ss << "SysInfo Type     : " << sysInfoType_ << endl;
                ss << "SysInfo CPU Num  : " << cpuNo_ << endl;
                ss << "SysInfo Mem Size : " << memorySize_ << endl;
                ss << "SysInfo Mac Addr : " << macAddressStr_ << endl;
                if ( sysInfoType_ )
                    ss << "SysInfo UUID     : " << UUID_ << endl;
                out << ss.str();
            }
        private:
            unsigned int cpuNo_;
            license_memory_size_t memorySize_;
            std::string UUID_;
            std::string macAddressStr_;
            bool sysInfoType_;
    }; // end - class SystemInfo



} // end - namespace license_tool

LICENSE_NAMESPACE_END

#endif // _LICENSE_TOOLS_H_

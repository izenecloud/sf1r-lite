///
/// @file LicenseTool.cpp
/// @brief source file of LicenseTool
/// @author Dohyun Yun
/// @date 2010-05-07
///

#include "LicenseTool.h"

#include <util/sysinfo/dmiparser.h>

#include <boost/date_time.hpp>
#include <boost/filesystem.hpp>

#include <limits.h>
#include <math.h>

#include <fstream>

#ifdef linux
#   include <sys/sysinfo.h>     // For system info
#   include <netinet/in.h>      // IP address
#   include <sys/ioctl.h>       // ioctl()
#   include <arpa/inet.h>       // inet_ntoa
#   include <net/if.h>          // struct ifconf ifreq
#   include <netinet/ether.h>   // ether_ntoa
#endif // linux

using namespace std;

LICENSE_NAMESPACE_BEGIN

namespace license_tool {

    int getCPUNo(void)
    {
        return sysconf(_SC_NPROCESSORS_CONF);
    } // end - getCPUNo()

    uint64_t getTotalRamSize(void)
    {
        struct sysinfo sys;
        sysinfo(&sys);
        return static_cast<uint64_t>(sys.totalram);
    } // end - getTotalRamSize()

    bool getMacIPAddrList(size_t& bArrSize, LICENSE_DATA_T& bArrData)
    {
        size_t tmpSize = 0;
        LICENSE_DATA_T tmpData(new unsigned char[SYSTEM_INFO_MAX_LENGTH]);

        // IP address and mac address list : This code supposes the maximum number of ethernets as 20.
        const int MAX_ETH_NO = 20;

        struct ifconf ifcfg; 
        memset(&ifcfg, 0, sizeof(ifcfg)); // memset is needed because ioctl() will fail without this line.
        ifcfg.ifc_len = sizeof(struct ifreq) * MAX_ETH_NO;
        ifcfg.ifc_buf = (char*)realloc( ifcfg.ifc_buf, ifcfg.ifc_len );

        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if ( sock == -1 ) {
            std::cerr << "License Socket Error" << std::endl;
            close(sock);
            return false;
        } else if ( ioctl(sock, SIOCGIFCONF, &ifcfg) < 0 ) {
            std::cerr << "License ioctl() Error" << std::endl;
            return false;
        }

        size_t ifreqSize = sizeof(struct ifreq);
        struct ifreq* ifr = ifcfg.ifc_req;
        for( int n = 0; n < ifcfg.ifc_len; n += ifreqSize, ifr++ )
        {
            struct sockaddr_in* sin = (struct sockaddr_in*) &ifr->ifr_addr;
            if ( ntohl(sin->sin_addr.s_addr) != INADDR_LOOPBACK )
            {
                // Copy IP address
                copyULongToBArr(sin->sin_addr.s_addr, tmpSize, tmpData);

                // Copy Mac address
                ioctl(sock, SIOCGIFHWADDR, ifr);
                struct sockaddr* sa = &( ifr->ifr_hwaddr );
                copyCArrToBArr(6, reinterpret_cast<unsigned char*>(sa->sa_data), tmpSize, tmpData);
            }
        } // end - for

        // Append tmpData to the end of bArrData
        copyCArrToBArr(tmpSize, tmpData.get(), bArrSize, bArrData);

        // Free allocated memory
        free(ifcfg.ifc_buf);
        close(sock);
        ifcfg.ifc_buf = NULL;

        return true;
    } // end - getMacIPAddrList()

    bool getUUID(size_t&bArrSize, LICENSE_DATA_T& bArrData)
    {
        using izenelib::util::sysinfo::DMIParser;
        DMIParser dmiParser(DMIParser::System);
        dmiParser.exec();
        dmiParser.setCurrentFrame(0);
        string UUID = dmiParser["UUID"];
        if ( UUID.length() != UUID_SIZE )
            return false;
        unsigned char encodedUUID[UUID_SIZE];
        for(size_t i = 0; i < UUID_SIZE; i++)
            encodedUUID[i] = static_cast<unsigned char>(UUID[i]) + UUID_ENCODE_VALUE;
        copyCArrToBArr(UUID_SIZE, encodedUUID, bArrSize, bArrData);
        return true;
    } // end - bool getUUID()

    bool getSystemInfo(size_t& sysInfoSize, LICENSE_DATA_T& sysInfo, bool includeUUID)
    {
        size_t tmpSize = 0;
        LICENSE_DATA_T tmpData( new unsigned char[SYSTEM_INFO_MAX_LENGTH] );

        // Add CPU No
        if( (tmpData[tmpSize++] = license_tool::getCPUNo()) == 0 )
            return false; // CPU No == 0

        // Add TotalRamSize
        if ( (license_tool::copyULongToBArr(license_tool::getTotalRamSize(), tmpSize, tmpData)) == 0 ) 
            return false;

        if ( includeUUID )
        {
            if ( !license_tool::getUUID(tmpSize, tmpData) )
            return false;
        }

        // Add Mac and IP address list
        if ( !license_tool::getMacIPAddrList(tmpSize, tmpData) )
            return false;

        // Add SystemInfo Type
        tmpData[tmpSize++] = static_cast<unsigned char>(includeUUID);

        sysInfoSize = tmpSize;
        sysInfo = tmpData;

        return true;
    } // end - getSystemInfo()

    size_t copyCArrToBArr(size_t cArrSize, unsigned char* cArrData,
            size_t& bArrSize, LICENSE_DATA_T& bArrData)
    {
        for(size_t i = 0; i < cArrSize; bArrSize++, i++)
            bArrData[bArrSize] = cArrData[i];
        return bArrSize;
    } // end - copyCArrToBArr()

    size_t copyULongToBArr(unsigned long uLongData, size_t& bArrSize, LICENSE_DATA_T& bArrData)
    {
        size_t ulSize = sizeof(unsigned long);
        memcpy(bArrData.get() + bArrSize, &uLongData, ulSize);
        bArrSize += ulSize; 
        return bArrSize;
    } // end - copyULongToBArr()

    bool getUCharArrayOfFile(const std::string& filePath, size_t& bArrSize, LICENSE_DATA_T& bArrData)
    {
        using namespace std;
        int len;
        LICENSE_DATA_T tmpData;
        try {
            ifstream fpin( filePath.c_str() , ios::binary );

            // Get length of file
            fpin.seekg ( 0 , ios::end );
            len = fpin.tellg();
            if ( len == -1 )
            {
                fpin.close();
                return false;
            }
            fpin.seekg ( 0, ios::beg );

            // Read binary data of given file.
            tmpData.reset(new unsigned char [len]);
            fpin.read((char*)tmpData.get(), len);
            fpin.close();
        } catch ( std::exception e ) {
            cerr << "getSha1HashOfFile() Exception : " << e.what() << endl;
            return false;
        }

        bArrData = tmpData;
        bArrSize = len;

        return true;
    } // end - getUCharArrayOfFile()
    bool getSha1HashOfFile(const std::string& filePath, LICENSE_DATA_T& bArrData)
    {
        using namespace std;
        size_t len;
        LICENSE_DATA_T tmpData;
        LICENSE_DATA_T tmpSha1Data(new unsigned char[20]);
        if ( !getUCharArrayOfFile(filePath, len, tmpData) )
            return false;
        sha1::calc(tmpData.get(), len, tmpSha1Data.get());
        bArrData = tmpSha1Data;
        return true;
    } // end - getSha1HashOfFile()


    bool LZO_Compress(const LICENSE_DATA_T& src, size_t srcLen, 
            LICENSE_DATA_T& tar, size_t& tarLen)
    {
        // if tar is not assigned yet, return false. tar MUST be assigned.
        if ( !tar )
            return false;

        static lzo_align_t __LZO_MMODEL
            wrkmem [((LZO1X_1_MEM_COMPRESS)+(sizeof(lzo_align_t)-1))/sizeof(lzo_align_t)];

        lzo_uint tmpTarLen;
        int ret = lzo1x_1_compress(src.get(), srcLen, tar.get(), &tmpTarLen, wrkmem);

        if ( ret != LZO_E_OK )
            return false;

        tarLen = tmpTarLen; // Range of tmpTarLen will not be over than the maxinum of size_t.

        return true;
    } // end - LZO_Compress()

    bool LZO_Decompress(const LICENSE_DATA_T& src, size_t srcLen, 
            LICENSE_DATA_T& tar, size_t& tarLen)
    {
        // if tar is not assigned yet, return false. tar MUST be assigned.
        if ( !tar )
            return false;

        lzo_uint tmpTarLen;
        int ret = lzo1x_decompress(src.get(), srcLen, tar.get(), &tmpTarLen, NULL);
        if (ret != LZO_E_OK)
            return false;
        tarLen = tmpTarLen; // Range of tmpTarLen will not be over than the maxinum of size_t.
        return true;
    } // end - LZO_Decompress()

    void LZO_setDefaultCompSize(size_t srcLen, LICENSE_DATA_T& tar)
    {
        tar.reset( new unsigned char[srcLen + ( srcLen / 1024 + 1 ) * 16] );
    } // end - LZO_setDefaultCompSize()

    bool SEED256_padding(size_t srcDataSize, size_t& newDataSize, boost::shared_array<BYTE>& encodedData, bool needResize)
    {
        size_t tmpSize = srcDataSize % SEED256_BLOCK_SIZE;
        if ( tmpSize == 0 )
        {
            newDataSize = srcDataSize;
            return false;
        }

        tmpSize = srcDataSize - tmpSize + SEED256_BLOCK_SIZE;

        if ( needResize )
        {
            boost::shared_array<BYTE> tmpData( new BYTE[tmpSize] );

            // copy encodedData to tmpData
            memcpy( tmpData.get(), encodedData.get(), srcDataSize);
            memset( tmpData.get() + srcDataSize, 0, tmpSize - srcDataSize);

            newDataSize = tmpSize;
            encodedData = tmpData;
        } // end - if
        else
            newDataSize = tmpSize;

        return true;
    } // end - SEED256_padding()

    bool SEED256_encode(const boost::shared_array<DWORD>& roundKey, size_t dataSize, boost::shared_array<BYTE>& encodedData)
    {
        // Error : Size is not fit to apply SEED 256 encryption. Run SEED256_padding before encryption
        // it is needed to apply padding if dataSize is not divided by 256 Bit.
        // SEED256 is 256bit block based encryption algorithm.
        if ( dataSize % SEED256_BLOCK_SIZE != 0)
            return false;

        size_t arrIdx = 0, leftSize = dataSize;

        boost::shared_array<BYTE> tmpData( new BYTE[dataSize] );

        // copy encodedData to tmpData
        memcpy( tmpData.get(), encodedData.get(), dataSize);

        // Apply encryption to the complete blocks ( every 256 bits = 36 bytes )
        while( leftSize >= SEED256_BLOCK_SIZE )
        {
            SeedEncrypt( tmpData.get() + arrIdx , roundKey.get() );
            leftSize -= SEED256_BLOCK_SIZE;
            arrIdx += SEED256_BLOCK_SIZE;
        } // end - while()

        encodedData = tmpData;

        return true;
    } // end - SEED_encode()

    bool SEED256_decode(const boost::shared_array<DWORD>& roundKey, size_t dataSize, boost::shared_array<BYTE>& decodedData)
    {
        size_t arrIdx = 0, leftSize = dataSize;

        boost::shared_array<BYTE> tmpData( new BYTE[dataSize] );
        memcpy( tmpData.get(), decodedData.get(), dataSize );

        // Apply decryption to the complete blocks ( every 256 bits = 36 bytes )
        while( leftSize >= SEED256_BLOCK_SIZE )
        {
            SeedDecrypt( tmpData.get() + arrIdx , roundKey.get() );
            leftSize -= SEED256_BLOCK_SIZE;
            arrIdx += SEED256_BLOCK_SIZE;
        } // end - while()

        if ( leftSize > 0 )
            return false;

        decodedData = tmpData;
        return true;

    } // end - SEED_decode()

    uint32_t getCurrentDate()
    {
        tm localTime = to_tm( boost::posix_time::second_clock::universal_time() );
        uint16_t year = localTime.tm_year + 1900;
        uint8_t month = static_cast<uint8_t>(localTime.tm_mon + 1);
        uint8_t day = static_cast<uint8_t>(localTime.tm_mday);
        uint32_t currentDate = year * 10000 + month * 100 + day;

        return currentDate;
    }
    uint32_t getStartDate(uint32_t date)
    {
        return date + license_module::encValueOfStartDate;
    }

    uint32_t getEndDate(uint32_t date)
    {
        return date + license_module::encValueOfEndDate;
    }

    void shuffleData(const tm& utcTime, size_t dataSize, LICENSE_DATA_T& data)
    {
        // TODO : Dohyun
    } // end - shuffleData()

    void unshuffleData(const tm& utcTime, size_t dataSize, LICENSE_DATA_T& data)
    {
        // TODO : Dohyun
    } // end - unshuffleData()

    bool cvtArrToStr(size_t inputSize, const LICENSE_DATA_T& inputData, std::string& outputStr)
    {
        char buf[2]={0,0};
        std::string tmpStr;

        for(size_t i = 0; i < inputSize; i++)
        {
            sprintf(buf, "%02X", static_cast<unsigned int>(inputData[i]));
            tmpStr += buf;
        }
        tmpStr.swap(outputStr);
        return true;
    } // end - cvtArrToStr()

    bool cvtStrToArr(const std::string& inputStr, size_t& outputSize, LICENSE_DATA_T& outputData)
    {
        char buf[3] = {0,0,0};
        size_t tmpSize = inputStr.size() / 2;
        boost::shared_array<unsigned char> tmpData(new unsigned char[tmpSize]);

        for(size_t i = 0; i < tmpSize; i++)
        {
            buf[0] = inputStr[i*2];
            buf[1] = inputStr[i*2 + 1];
            sscanf(buf, "%02X", (unsigned int*)(&tmpData[i]));
        }
        outputSize = tmpSize;
        outputData = tmpData;
        return true;
    } // end - cvtStrToArr()

    uintmax_t dirSize(const std::string& dirPath, bool recursive)
    {
        using namespace boost::filesystem;
        try {
            if ( !exists(dirPath) )
                return 0;

            uintmax_t tmpSize = 0;
            directory_iterator dirIter = directory_iterator(dirPath);
            directory_iterator dirIterEnd;
            for(; dirIter != dirIterEnd; dirIter++)
            {
                if ( is_directory(*dirIter) )
                {
                    if ( recursive )
                        tmpSize += dirSize(dirIter->string(), true);
                    continue;
                }
                tmpSize += file_size(*dirIter);
            }
            return tmpSize;
        } catch ( filesystem_error e ) {
            cerr << "Exception : " << e.what() << endl;
            return 0;
        }
    }

    void printLicenseData(const std::string& title, size_t licenseSize, const LICENSE_DATA_T& licenseData, ostream& out)
    {
        std::string tmpStr; 
        license_tool::cvtArrToStr(licenseSize, licenseData, tmpStr); 
        out << "-----[ " << title << " : " << tmpStr << " , " << licenseSize << " ]" << std::endl;
    } // end - printLicenseData()
} // end - license_tool

LICENSE_NAMESPACE_END


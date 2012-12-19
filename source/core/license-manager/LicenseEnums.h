///
/// @file   LicenseEnums.h
/// @brief
/// @author Dohyun Yun ( dualistmage@gmail.com )
/// @date
///

#ifndef _LICENSE_ENUMS_H_
#define _LICENSE_ENUMS_H_

#include "encryption/SEED_256_KISA.h"

#include <boost/shared_array.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>

#include <iostream>

#define LICENSE_NAMESPACE_BEGIN namespace license_module {
#define LICENSE_NAMESPACE_END }

LICENSE_NAMESPACE_BEGIN

    // Typedefs
    typedef uint64_t product_info_t;
    typedef uint32_t sysinfo_size_t;
    typedef uint32_t custinfo_size_t;
    typedef uint32_t license_date_t;

    typedef uint8_t license_concurrency_t;
    typedef uint64_t license_max_doc_t;
    typedef uintmax_t license_max_index_data_size_t;

    typedef uint64_t license_memory_size_t;


    const size_t LICENSE_INFO_LIST_SIZE = 3;
    const size_t SYSTEM_INFO_MAX_LENGTH = 50;
    const size_t LICENSE_INFO_MAX_LENGTH = 320;

    const size_t LICENSE_DATE_SIZE = sizeof(uint32_t) * 2;
    const size_t LICENSE_DATE_START = 0;
    const size_t LICENSE_DATE_END = 1;
    const size_t LICENSE_TOTAL_TIME_SIZE = sizeof(uint32_t);
    const size_t LICENSE_TIME_LEFT_SIZE = sizeof(uint32_t);

    const size_t LICENSE_DOC_LIMIT_SIZE = sizeof(uint64_t) * 2;
    const size_t LICENSE_MAX_DOC_NUM = 0;
    const size_t LICENSE_MAX_DOC_SIZE = 1;

    const int LICENSE_NETWORK_VALIDATION_SUCCESS =  1;
    const int LICENSE_NETWORK_VALIDATION_SKIPPED =  0;
    const int LICENSE_NETWORK_VALIDATION_FAILED  = -1;

    // Encryption
    const size_t SEED256_BLOCK_SIZE = 32; // byte size
    const size_t SEED256_ROUNDKEY_SIZE = 48; 
    const size_t SEED256_ROUNDKEY_BYTE_SIZE = SEED256_ROUNDKEY_SIZE * sizeof(DWORD); // byte size

    // Packet Data Size
    // Client Packet Structure : [PACKET_HEADER] [REQ_HEADER]
    const size_t PACKET_HEADER_SIZE = 8;
    const size_t PRODUCT_CODE_SIZE = sizeof(product_info_t); // byte size
    const size_t SYSINFOSIZE_SIZE = sizeof(sysinfo_size_t); // byte size
    const size_t CUSTINFOSIZE_SIZE = sizeof(custinfo_size_t); // byte size
    const size_t REQ_HEADER_SIZE = 1; // byte size
    const size_t ETC_DATA_SIZE = 8; // byte size
    const size_t DATE_SIZE = 8; // byte size
    const size_t UUID_SIZE = 36; // byte size

    typedef unsigned char LICENSE_DATA_UNIT_T;
    typedef boost::shared_array<LICENSE_DATA_UNIT_T> LICENSE_DATA_T;

    typedef boost::shared_ptr<boost::asio::ip::tcp::socket> SOCK_PTR_T;

    enum LICENSE_REQUEST_TYPE {
        REQ_MIN = 0,
        REQ_CERTIFICATE,
        REQ_LICENSE_FILE,
        REQ_INSERT_LICENSE_DATA,
        REQ_DELETE_LICENSE_DATA,
        REQ_UPDATE_LICENSE_DATA,
        REQ_MAX
    };

    const license_date_t encValueOfStartDate  = 20100910;
    const license_date_t encValueOfEndDate    = 20100820;

    const unsigned char UUID_ENCODE_VALUE = 96;

LICENSE_NAMESPACE_END

#endif // _LICENSE_ENUMS_H_

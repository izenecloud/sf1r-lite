///
/// @file   LicenseEncryptor.cpp
/// @brief  A source file of license encryptor which does compression-encryption and vise versa.
/// @author Dohyun Yun ( dualistmage@gmail.com )
/// @date
///

#include "LicenseEncryptor.h"

LICENSE_NAMESPACE_BEGIN

LicenseEncryptor::LicenseEncryptor():
    userKey_(new BYTE[SEED256_BLOCK_SIZE])
{
    // Set round key which is used encryption and decryption.
    for(size_t i = 0; i < SEED256_BLOCK_SIZE; i++)
        userKey_[i] = i;
    userKey_[13] = 0;
} // end - LicenseEncryptor()

bool LicenseEncryptor::encryptData(size_t inputSize, const LICENSE_DATA_T& inputData,
        size_t& outputSize, LICENSE_DATA_T& outputData)
{
    using namespace std;

    LICENSE_DATA_T tmpSha( new unsigned char[20] );
    LICENSE_DATA_T tmpData( new unsigned char[LICENSE_INFO_MAX_LENGTH] );
    size_t tmpSize = 20 + inputSize;

    // Copy input Data into tmpData
    memcpy(tmpData.get() + 20, inputData.get(), inputSize);

    // Calculate sha1 of tmpData( sha1 Range : tmpData[20] ~ tmpData[20 + inputSize] )
    sha1::calc(tmpData.get() + 20, inputSize, tmpSha.get());

    // Copy inputData to the tail of tmpData
    memcpy( tmpData.get(), tmpSha.get(), 20 );

    // SEED256 Encryption
    license_tool::SEED256_padding( tmpSize, tmpSize, tmpData );

    boost::shared_array<DWORD> roundKey(new DWORD[SEED256_ROUNDKEY_SIZE]);
    SeedRoundKey(roundKey.get(), userKey_.get());
    license_tool::SEED256_encode( roundKey, tmpSize, tmpData );
    {
        LICENSE_DATA_T tmpDec(new unsigned char[tmpSize]);
        memcpy(tmpDec.get(), tmpData.get(), tmpSize);
        license_tool::SEED256_decode( roundKey, tmpSize, tmpDec);
    }

    // LZO Compression
    size_t tmpOutSize;
    LICENSE_DATA_T tmpOutData(new unsigned char[LICENSE_INFO_MAX_LENGTH + SEED256_ROUNDKEY_BYTE_SIZE]);
    license_tool::LZO_Compress(tmpData, tmpSize, tmpOutData, tmpOutSize);

    // Add original data length at tail
    memcpy( tmpOutData.get() + tmpOutSize, &inputSize, sizeof(size_t) );
    tmpOutSize += sizeof(size_t);

    // Add round key at tail
    memcpy( tmpOutData.get() + tmpOutSize, roundKey.get(), SEED256_ROUNDKEY_BYTE_SIZE );
    tmpOutSize += SEED256_ROUNDKEY_BYTE_SIZE;

    outputSize = tmpOutSize;
    outputData = tmpOutData;

    return true;
} // end - encryptData()

bool LicenseEncryptor::decryptData(size_t inputSize, const LICENSE_DATA_T& inputData,
        size_t& outputSize, LICENSE_DATA_T& outputData)
{
    using namespace std;

    size_t tmpSize, originalDataSize;
    LICENSE_DATA_T tmpData( new unsigned char[LICENSE_INFO_MAX_LENGTH] );
    
    // Extract round key
    boost::shared_array<DWORD> roundKey(new DWORD[SEED256_ROUNDKEY_SIZE]);
    memcpy(roundKey.get(), inputData.get() + inputSize - SEED256_ROUNDKEY_BYTE_SIZE, SEED256_ROUNDKEY_BYTE_SIZE);
    inputSize -= SEED256_ROUNDKEY_BYTE_SIZE;
    
    // Extract original string length
    memcpy(&originalDataSize, inputData.get() + inputSize - sizeof(size_t), sizeof(size_t) );
    inputSize -= sizeof(size_t);

    // LZO Decompression
    license_tool::LZO_Decompress(inputData, inputSize, tmpData, tmpSize);

    // SEED256 Decoding
    license_tool::SEED256_decode(roundKey, tmpSize, tmpData);

    // Compare sha1 in input Data with calculated sha1
    LICENSE_DATA_T tmpSha(new unsigned char[20]);
    sha1::calc(tmpData.get() + 20, originalDataSize, tmpSha.get());

    if (memcmp(tmpSha.get(), tmpData.get(), 20))
        return false;

    outputSize = originalDataSize;
    outputData.reset( new unsigned char[outputSize] );
    memcpy(outputData.get(), tmpData.get() + 20, outputSize);

    return true;
} // end - decryptData()

LICENSE_NAMESPACE_END

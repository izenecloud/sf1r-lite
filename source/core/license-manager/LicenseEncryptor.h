///
/// @file   LicenseEncryptor.h
/// @brief  A header file of license encryptor which does compression-encryption and vise versa.
/// @author Dohyun Yun ( dualistmage@gmail.com )
/// @date
///

#ifndef _LICENSE_ENCRYPTOR_H_
#define _LICENSE_ENCRYPTOR_H_

#include "LicenseEnums.h"
#include "LicenseTool.h"

LICENSE_NAMESPACE_BEGIN

class LicenseEncryptor
{
    public:

        ///
        /// @brief a constructor of License Encryptor
        ///
        LicenseEncryptor();

        ///
        /// @brief This interface encrypts input data.
        /// @details
        ///     -# Resize data size to apply encryption padding.
        ///     -# Attach SHA1 hash to the header. ( Sha1 Range : tmpData[20] ~ tmpData[tmpSize] )
        ///     -# Apply SEED256 Encryption.
        ///     -# Shuffle data according to the time information.
        ///     -# Compression
        /// @param[inputSize] size of input data.
        /// @param[inputData] data array of input.
        /// @param[encryptTime] Encryption posix time.
        /// @param[encryptDataSize] size of output data.
        /// @param[encryptData] data array of output.
        /// @return true if success or false.
        ///
        bool encryptData(size_t inputSize, const LICENSE_DATA_T& inputData,
                size_t& encryptDataSize, LICENSE_DATA_T& encryptData);

        ///
        /// @brief This interface decodes license Data. which is the reverse way of encryptData().
        /// @param[encryptedData] is an input variable which contains encrypted data of license.
        /// @param[decryptedData] is an output variable which contains decrypted data of license.
        ///                     and will contain decrypted value after.
        /// @return true if success or false.
        ///
        bool decryptData(size_t inputSize, const LICENSE_DATA_T& inputData,
                size_t& originalDataSize, LICENSE_DATA_T& decryptData);

    private:
        ///
        /// @brief a private key which is used to generate roundKey_
        ///
        boost::shared_array<BYTE> userKey_;

}; // end - class LicenseEncryptor

LICENSE_NAMESPACE_END

#endif // _LICENSE_ENCRYPTOR_H_

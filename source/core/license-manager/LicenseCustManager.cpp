/*
 * LicenseCustManager.cpp
 *
 *  Created on: Dec 13, 2012
 *      Author: Xin Liu
 */

#include "LicenseCustManager.h"
#include "LicenseEncryptor.h"

namespace sf1r
{

void LicenseCustManager::setCollectionInfo()
{
	if (!newCollectionList_.empty())
		newCollectionList_.clear();

	size_t tmpSize, licenseSize, offset;
	LICENSE_DATA_T tmpData, licenseData;
	if ( !license_tool::getUCharArrayOfFile(licenseFilePath_, tmpSize, tmpData) )
		return;

	// Decryption
	LicenseEncryptor licenseEncryptor;
	licenseEncryptor.decryptData(tmpSize, tmpData, licenseSize, licenseData);

	offset = license_module::LICENSE_TOTAL_TIME_SIZE
			+ license_module::LICENSE_TIME_LEFT_SIZE;
	if (licenseSize > (offset + sizeof(uint32_t)))
	{
		size_t custNum = 0;
		memcpy(&custNum, licenseData.get() + offset, sizeof(uint32_t));
		offset += sizeof(uint32_t);

		if(custNum > 0)
		{
			// Extract customer info
			custinfo_size_t custInfoSize;
			license_tool::CustomerInfo custInfo;

			for(size_t i = 0; i < custNum; i++)
			{
				memcpy(&custInfoSize, licenseData.get() + offset, sizeof(custinfo_size_t));
				offset += sizeof(custinfo_size_t);
				custInfo.init(offset, custInfoSize, licenseData);
				offset += custInfoSize;
				std::string collectionName = custInfo.getCollectionName();
				uint32_t startDate = custInfo.getStartDate();
				uint32_t endDate = custInfo.getEndDate();
				uint32_t currentDate = license_tool::getCurrentDate();
				if (endDate < currentDate)
					continue;
				if (!hasCollection(collectionName))
				{
					collectionDateMap_[collectionName] = std::pair<uint32_t, uint32_t>(startDate, endDate);
					newCollectionList_.push_back(collectionName);
				}
			}
		}
	}
}

}


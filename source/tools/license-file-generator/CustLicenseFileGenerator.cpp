/*
 * CustLicenseFileGenerator.cpp
 *
 *  Created on: Dec 7, 2012
 *      Author: Xin Liu
 */

#include <license-manager/LicenseManager.h>
#include <license-manager/LicenseRequestFileGenerator.h>
#include <util/sysinfo/dmiparser.h>

#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;
using namespace sf1r;
using namespace license_module;
using namespace license_tool;

void getLicenseFilename(const std::string& message, std::string& output);
void checkDate(uint32_t& date);
bool getNewCustInfo(std::string& collectionName, uint32_t& startDate, uint32_t& endDate, uint32_t& leftTime);
bool extractTotalTimeNTimeLeft(const LICENSE_DATA_T& licenseData, size_t& timeOffest, uint32_t& totalTime, uint32_t& leftTime);
bool custInfoToArr(uint32_t custInfoSize, const std::string& collectionName, uint32_t startDate, uint32_t endDate, LICENSE_DATA_T& licenseData, size_t& licenseSize);
void grow(size_t licenseSize, size_t increasedSize, size_t& maxSize, LICENSE_DATA_T& licenseData);

int main()
{
	string licenseFileName;
	size_t tmpSize, licenseSize;
	LICENSE_DATA_T tmpData, licenseData;
	size_t offset = 0;
	uint32_t totalTime, leftTime;
	uint32_t customerNum = 0;
	bool hasCustInfo = false;

	getLicenseFilename("Enter License File Name", licenseFileName);
    if ( !license_tool::getUCharArrayOfFile(licenseFileName, tmpSize, tmpData) ) {
        cout << "Read Error : " << licenseFileName << endl;
        return 1;
    }

    // Print current license file contents
    LicenseRequestFileGenerator::printLicenseFileInFo(licenseFileName);

	LicenseEncryptor licenseEncryptor;
	licenseEncryptor.decryptData(tmpSize, tmpData, licenseSize, licenseData);

	// Extract the total usable time and time left
	while(!extractTotalTimeNTimeLeft(licenseData, offset, totalTime, leftTime));

	// Check if there are customer info or not
	if (licenseSize > offset)
	{
		hasCustInfo = true;
		memcpy(&customerNum, licenseData.get() + offset, sizeof(uint32_t));
		offset += sizeof(uint32_t);
	}

	size_t maxSize = LICENSE_INFO_MAX_LENGTH; // allocated memory
	{
		LICENSE_DATA_T tmpData(new LICENSE_DATA_UNIT_T[maxSize]);
		memcpy(tmpData.get(), licenseData.get(), licenseSize);
		licenseData = tmpData;
	}
    if (!hasCustInfo)
    {
    	// Append customer number to license data
    	if ((licenseSize + sizeof(uint32_t)) > maxSize)
    		grow(licenseSize, sizeof(uint32_t), maxSize, licenseData);
    	//std::cout<<"maxSize = "<<maxSize<<std::endl;
    	memcpy(licenseData.get() + licenseSize, &customerNum, sizeof(uint32_t));
    	licenseSize += sizeof(uint32_t);
    	//std::cout<<"licenseSize = "<<licenseSize<<std::endl;
    }

	while(leftTime > 0)
	{
		std::string input;
		cout << "Assign sf1r use time to a new customer? " << " (q for exit, other char for continue) : ";
		cin >> input;

		if( input == "q")
			break;

		uint32_t custInfoSize;
		std::string collectionName;
		uint32_t startDate, endDate;
		while(!getNewCustInfo(collectionName, startDate, endDate, leftTime));
		custInfoSize = collectionName.size() + sizeof(uint32_t) * 2;
		if ((licenseSize + custInfoSize + sizeof(uint32_t)) > maxSize)
			grow(licenseSize, custInfoSize + sizeof(uint32_t), maxSize, licenseData);
		custInfoToArr(custInfoSize, collectionName, startDate, endDate, licenseData, licenseSize);
		customerNum++;
		cout <<"Time Left (months):"<<leftTime<<std::endl;
	}

    if (customerNum > 0)
    {// reset customer num
    	memcpy(licenseData.get() + LICENSE_TOTAL_TIME_SIZE + LICENSE_TIME_LEFT_SIZE, &customerNum, sizeof(uint32_t));
    	memcpy(licenseData.get() + LICENSE_TOTAL_TIME_SIZE, &leftTime, LICENSE_TIME_LEFT_SIZE);
    }
	// Encrypt license data
	size_t encSize;
    LICENSE_DATA_T encData;
	licenseEncryptor.encryptData(licenseSize, licenseData, encSize, encData);

	// Write data into file
	ofstream fpout(licenseFileName.c_str());
	fpout.write( (char*)(encData.get()), encSize );
	fpout.close();
	LicenseRequestFileGenerator::printLicenseFileInFo(licenseFileName);
}

void grow(size_t licenseSize, size_t increasedSize, size_t& maxSize, LICENSE_DATA_T& licenseData)
{
	size_t newSize = licenseSize + increasedSize;
	size_t newMax = maxSize;
	while(newMax < newSize)
		newMax *= 2;

	LICENSE_DATA_T newLicenseData(new LICENSE_DATA_UNIT_T[newMax]);
	memcpy(newLicenseData.get(), licenseData.get(), licenseSize);
	licenseData = newLicenseData;
	maxSize = newMax;
}

bool custInfoToArr(uint32_t custInfoSize, const std::string& collectionName, uint32_t startDate, uint32_t endDate,
		LICENSE_DATA_T& licenseData, size_t& licenseSize)
{
	memcpy(licenseData.get() + licenseSize, &custInfoSize, sizeof(uint32_t));
	licenseSize += sizeof(uint32_t);
	memcpy(licenseData.get() + licenseSize, &startDate, sizeof(uint32_t));
	licenseSize += sizeof(uint32_t);
	memcpy(licenseData.get() + licenseSize, &endDate, sizeof(uint32_t));
	licenseSize += sizeof(uint32_t);

	char buf[2] = {0,0};
	size_t tmpSize = collectionName.size();
	for(size_t i = 0; i < tmpSize; licenseSize++, i++)
	{
		buf[0] = collectionName[i];
		sscanf(buf, "%c", &licenseData[licenseSize]);
	}
	return true;
}

bool extractTotalTimeNTimeLeft(const LICENSE_DATA_T& licenseData, size_t& timeOffset, uint32_t& totalTime,
		uint32_t& leftTime)
{
	// Extract total time
	memcpy(&totalTime, licenseData.get()+ timeOffset, LICENSE_TOTAL_TIME_SIZE);
	timeOffset += LICENSE_TOTAL_TIME_SIZE;
	// Extract left time
	memcpy(&leftTime, licenseData.get() + timeOffset, LICENSE_TIME_LEFT_SIZE);
	timeOffset += LICENSE_TIME_LEFT_SIZE;
	return true;
} // end - extractTotalTimeNTimeLeft()

bool getNewCustInfo(std::string& collectionName, uint32_t& startDate, uint32_t& endDate, uint32_t& leftTime)
{
	std::cout<< "------------------------------" << std::endl;
	std::cout<< "Enter customer information:" << std::endl;
	uint32_t assignTime, currentDate;
	currentDate = license_tool::getCurrentDate();
	std::cout<<"Current date : "<< currentDate <<std::endl;
	cout << "Collection Name (String Type) :"; cin >> collectionName;
	cout << "Start Date (Format:YYYYMMDD) : "; cin >> startDate;
	if (startDate < currentDate)
	{
		std::cout<<"Error: start date must be no less than current date("<< currentDate<<")"<<std::endl;
		std::cout<<"Re-enter customer info: "<< std::endl;
		return false;
	}
	cout << "Assigned Time (months): "; cin >> assignTime;
	if (assignTime > leftTime)
	{
		std::cout <<"Error: the assigned time must be no greater than "<<leftTime<<" months"<<std::endl;
		std::cout<<"Re-enter customer info: "<<std::endl;
		return false;
	}

	uint32_t interval;
	{
		if (assignTime > 12)
		{
			size_t year = assignTime / 12;
			size_t month = assignTime - year * 12;
			interval = year * 10000 + month * 100;
		}
		else
			interval = assignTime * 100;
	}
	endDate = startDate + interval;
	checkDate(endDate);
	cout<<"Start Date: "<<startDate<<" ~ End Date: "<<endDate<<std::endl;
	leftTime -= assignTime;
	return true;
}

void checkDate(uint32_t& date)
{
	uint32_t tmpDate = date;
	uint16_t year = tmpDate / 10000;
    uint8_t month = (tmpDate - year * 10000) / 100;
	uint8_t day = tmpDate - year * 10000 - month * 100;

	if (month > 12)
	{
		size_t num = month / 12;
		month -= num * 12;
		if (month == 0)
		{
			num--;
			month = 12;
		}
		year += num;
	}
	date = year * 10000 + month * 100 + day;
}

void getLicenseFilename(const std::string& message, std::string& output)
{
	std::string input;
	std::string fileName = LicenseManager::LICENSE_KEY_FILENAME;
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
} // end - getLicenseFilename()



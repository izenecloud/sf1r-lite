/*
 * LicenseCustManager.h
 *
 *  Created on: Dec 13, 2012
 *      Author: Xin Liu
 */

#ifndef LICENSECUSTMANAGER_H_
#define LICENSECUSTMANAGER_H_

#include "LicenseEnums.h"
#include "LicenseTool.h"

#include <util/singleton.h>

#include <map>
#include <string>
#include <vector>

using namespace license_module;

namespace sf1r
{

class LicenseCustManager
{
	typedef std::map<std::string, std::pair<uint32_t,uint32_t> >::const_iterator date_const_iterator;

public:
	LicenseCustManager() {}

	~LicenseCustManager() {}

public:
	static LicenseCustManager* get()
	{
		return ::izenelib::util::Singleton<LicenseCustManager>::get();
	}

	void setLicenseFilePath(std::string path)
	{
		std::cout<<"set license file path: "<<path<<std::endl;
		licenseFilePath_ = path;
	}

	bool hasCollection(const std::string& collectionName) const
	{
		if (collectionDateMap_.find(collectionName) != collectionDateMap_.end())
			return true;
		return false;
	}

	bool getDate(const std::string collectionName, std::pair<uint32_t, uint32_t>& date) const
	{
		if (!hasCollection(collectionName))
			return false;
		date_const_iterator iter = collectionDateMap_.find(collectionName);
		date = iter->second;
		return true;
	}

	void setCollectionInfo();

	const std::vector<std::string> getCollectionList() const
	{
		return newCollectionList_;
	}

	void deleteCollection(std::string collection)
	{
		collectionDateMap_.erase(collection);
	}

private:
	std::map<std::string, std::pair<uint32_t, uint32_t> > collectionDateMap_;
	std::string licenseFilePath_;
	std::vector<std::string> newCollectionList_;

}; // end - class LicenseCustManager

} // end - namespace sf1r

#endif /* LICENSECUSTMANAGER_H_ */

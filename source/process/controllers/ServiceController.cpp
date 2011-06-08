/**
 * \file ba-process/controllers/ServiceController.cpp
 * \date Jun 1, 2011
 * \author Xin Liu
 */

#include "ServiceController.h"
#include <common/XmlConfigParser.h>
#include <boost/filesystem.hpp>

#include <fstream>
#include <map>

namespace sf1r {

void ServiceController::process_overdue()
{
#ifdef  EXIST_LICENSE
    char* home = getenv("HOME");
    std::string licenseDir = home; licenseDir += "/sf1-license/";
    // Check whether license directory is exist
   // if ( !boost::filesystem::exists(licenseDir) )
    //{
      //  return;
    //}

    /// Write token file
    std::string path = licenseDir + LicenseManager::TOKEN_FILENAME;
    std::string tokenCodeStr = LicenseManager::STOP_SERVICE_TOKEN;
    LicenseManager::write_token_to(path, tokenCodeStr);

    ///Insert the "@@ALL@@" token into the deny control lists for all collections
    std::map<std::string, CollectionMeta>&
        collectionMetaMap = SF1Config::get()->mutableCollectionMetaMap();
    std::map<std::string, CollectionMeta>::iterator
        collectionIter = collectionMetaMap.begin();

    for(; collectionIter != collectionMetaMap.end(); collectionIter++)
    {
        CollectionMeta& collectionMeta = collectionIter->second;
        collectionMeta.deleteTokenFromAclDeny(LicenseManager::START_SERVICE_TOKEN);
        collectionMeta.aclDeny(LicenseManager::STOP_SERVICE_TOKEN);
    }
#endif
}

void ServiceController::renew()
{
#ifdef  EXIST_LICENSE
    char* home = getenv("HOME");
    std::string licenseDir = home; licenseDir += "/sf1-license/";
    // Check whether license directory is exist
    //if ( !boost::filesystem::exists(licenseDir) )
    //{
      //  return;
    //}

    // Write token file
    std::string path = licenseDir + LicenseManager::TOKEN_FILENAME;
    std::string tokenCodeStr = LicenseManager::START_SERVICE_TOKEN;
    LicenseManager::write_token_to(path, tokenCodeStr);

    /// Insert the "@@NONE@@"token into the deny control lists of all collections
    std::map<std::string, CollectionMeta>&
        collectionMetaMap = SF1Config::get()->mutableCollectionMetaMap();
    std::map<std::string, CollectionMeta>::iterator
        collectionIter = collectionMetaMap.begin();

    for(; collectionIter != collectionMetaMap.end(); collectionIter++)
    {
        CollectionMeta& collectionMeta = collectionIter->second;
        collectionMeta.deleteTokenFromAclDeny(LicenseManager::STOP_SERVICE_TOKEN);
        collectionMeta.aclDeny(LicenseManager::START_SERVICE_TOKEN);
    }
#endif
}

}

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

/**
 * @brief Action @b process_overdue. This API is invoked when the service
 * fee is overdued.
 */
void ServiceController::process_overdue()
{
#ifdef  EXIST_LICENSE
    char* home = getenv("HOME");
    std::string licenseDir = home; licenseDir += "/sf1-license/";

    /// Write token file
    std::string path = licenseDir + LicenseManager::TOKEN_FILENAME;
    std::string tokenCodeStr = LicenseManager::STOP_SERVICE_TOKEN;
    LicenseManager::write_token_to(path, tokenCodeStr);

    ///Insert the "@@ALL@@" token into the deny control lists for all collections
    SF1Config::CollectionMetaMap& collectionMetaMap = SF1Config::get()->mutableCollectionMetaMap();
    SF1Config::CollectionMetaMap::iterator collectionIter = collectionMetaMap.begin();

    for(; collectionIter != collectionMetaMap.end(); collectionIter++)
    {
        CollectionMeta& collectionMeta = collectionIter->second;
        collectionMeta.deleteTokenFromAclDeny(LicenseManager::START_SERVICE_TOKEN);
        collectionMeta.aclDeny(LicenseManager::STOP_SERVICE_TOKEN);
    }
#endif
}

/**
 * @brief Action @b process_overdue. When the overdue fees is paid, this API is invoked
 * to renew service.
 */
void ServiceController::renew()
{
#ifdef  EXIST_LICENSE
    char* home = getenv("HOME");
    std::string licenseDir = home; licenseDir += "/sf1-license/";

    // Write token file
    std::string path = licenseDir + LicenseManager::TOKEN_FILENAME;
    std::string tokenCodeStr = LicenseManager::START_SERVICE_TOKEN;
    LicenseManager::write_token_to(path, tokenCodeStr);

    /// Insert the "@@NONE@@"token into the deny control lists of all collections
    SF1Config::CollectionMetaMap& collectionMetaMap = SF1Config::get()->mutableCollectionMetaMap();
    SF1Config::CollectionMetaMap::iterator collectionIter = collectionMetaMap.begin();

    for(; collectionIter != collectionMetaMap.end(); collectionIter++)
    {
        CollectionMeta& collectionMeta = collectionIter->second;
        collectionMeta.deleteTokenFromAclDeny(LicenseManager::STOP_SERVICE_TOKEN);
        collectionMeta.aclDeny(LicenseManager::START_SERVICE_TOKEN);
    }
#endif
}

}

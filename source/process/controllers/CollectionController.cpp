/**
 * \file CollectionController.cpp
 * \brief 
 * \date Dec 20, 2011
 * \author Xin Liu
 */

#include "CollectionController.h"

#include <process/common/CollectionManager.h>
#include <process/common/XmlConfigParser.h>

#include <iostream>

namespace sf1r
{

void CollectionController::start_collection()
{
    std::string collection = asString(request()[Keys::collection]);
    if (collection.empty())
    {
        response().addError("Require field collection in request.");
        return;
    }
    std::string configFile = asString(request()[Keys::config_file]);
    if (configFile.empty())
    {
        response().addError("Require collection config file in request.");
        return;
    }
    CollectionManager::get()->startCollection(collection, configFile);
}

void CollectionController::stop_collection()
{
    std::string collection = asString(request()[Keys::collection]);
    if (collection.empty())
    {
        response().addError("Require field collection in request.");
        return;
    }
    if (!SF1Config::get()->checkCollectionAndACL(collection, request().aclTokens()))
    {
        response().addError("Collection access denied");
        return;
    }
    CollectionManager::get()->stopCollection(collection);
}

} //namespace sf1r


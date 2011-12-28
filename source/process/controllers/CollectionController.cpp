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

/**
 * @brief Action @b start_collection.
 *
 * @section request
 *
 * - @b collection* (@c String): Collection name.
 *
 * @section response
 *
 * - No extra fields.
 *
 * @section example
 *
 * Request
 *
 * @code
 * {
 *   "collection": "chwiki"
 * }
 * @endcode
 *
 */
void CollectionController::start_collection()
{
    std::string collection = asString(request()[Keys::collection]);
    if (collection.empty())
    {
        response().addError("Require field collection in request.");
        return;
    }
    std::string configFile = SF1Config::get()->getHomeDirectory();
    std::string slash("");
#ifdef WIN32
        slash = "\\";
#else
        slash = "/";
#endif
    configFile += slash + collection + ".xml";
    CollectionManager::get()->startCollection(collection, configFile);
}

/**
 * @brief Action @b stop_collection.
 *
 * @section request
 *
 * - @b collection* (@c String): Collection name.
 *
 * @section response
 *
 * - No extra fields.
 *
 * @section example
 *
 * Request
 *
 * @code
 * {
 *   "collection": "chwiki"
 * }
 * @endcode
 *
 */
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


/**
 * @file process/controllers/Sf1Controller.cpp
 * @author Ian Yang
 * @date Created <2011-01-25 18:40:12>
 */
#include "Sf1Controller.h"
#include <common/Keys.h>
#include <common/XmlConfigParser.h>
#include <common/CollectionManager.h>

namespace sf1r
{

using driver::Keys;
using namespace izenelib::driver;

Sf1Controller::Sf1Controller()
        : collectionHandler_(0)
{
}


Sf1Controller::~Sf1Controller()
{
}

/**
 * User tokens has been get from request["header"]["acl_tokens"]. The collection
 * ACL rule is read from configuration file.
 *
 * If the request has set the collection but the collection is not allowed to
 * the user according to the access tokens, an error is returned.
 */
bool Sf1Controller::checkCollectionAcl()
{
    Value::StringType collection = asString(
                                       this->request()[Keys::collection]
                                   );
    if (!SF1Config::get()->checkCollectionAndACL(collection, this->request().aclTokens()))
    {
        this->response().addError("Collection access denied");
        return false;
    }

    collectionHandler_ = CollectionManager::get()->findHandler(collection);
    return true;
}

} // namespace sf1r

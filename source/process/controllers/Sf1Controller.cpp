/**
 * @file process/controllers/Sf1Controller.cpp
 * @author Ian Yang
 * @date Created <2011-01-25 18:40:12>
 */
#include "Sf1Controller.h"
#include "CollectionHandler.h"
#include <bundles/index/IndexTaskService.h>
#include <node-manager/MasterManagerBase.h>
#include <node-manager/RequestLog.h>
#include <node-manager/DistributeRequestHooker.h>

#include <util/driver/writers/JsonWriter.h>

#include <common/Keys.h>
#include <common/XmlConfigParser.h>

namespace
{
const char* REQUIRE_COLLECTION_NAME = "Require field collection in request.";
}

namespace sf1r
{

using driver::Keys;
using namespace izenelib::driver;

Sf1Controller::Sf1Controller(bool requireCollectionName)
    : collectionHandler_(0)
    , mutex_(0)
    , requireCollectionName_(requireCollectionName)
{
}

Sf1Controller::~Sf1Controller()
{
    if(mutex_)
    {
        mutex_->unlock_shared();
    }
}

bool Sf1Controller::preprocess()
{
    std::string error;

    if (doCollectionCheck(error) || (!requireCollectionName_ && collectionName_.empty()))
    {
        // need call in distribute, so we push it to queue and return false to ignore this request.
        if(callDistribute())
            return false;
        // hook request if needed.
        if (request().callType() == Request::FromAPI)
        {
            // from api do not need hook, just process as usually. all read only request 
            // will return from here.
            return true;
        }
        //const std::string& reqdata = DistributeRequestHooker::get()->getAdditionData();
        //DistributeRequestHooker::get()->hookCurrentReq(reqdata);
        //DistributeRequestHooker::get()->processLocalBegin();
        // note: if the request need to shard to different node.
        // you should do hook again to all shard before you actually do the processing.
        return true;
    }

    response().addError(error);

    if (request().callType() != Request::FromAPI)
    {
        std::string errinfo;
        try
        {
            errinfo = toJsonString(response()[Keys::errors]);
        }
        catch(...)
        {
        }
        std::cout << "request failed before send!!" << errinfo << std::endl;
        DistributeRequestHooker::get()->processFinishedBeforePrepare(false);
    }

    return false;
}

void Sf1Controller::postprocess()
{
    if (request().callType() != Request::FromAPI)
    {
        if (!response().success())
        {
            std::string errinfo;
            try
            {
                errinfo = toJsonString(response()[Keys::errors]);
            }
            catch(...)
            {
            }
            std::cout << "request failed before send!!" << errinfo << std::endl;
            DistributeRequestHooker::get()->processLocalFinished(false);
        }
        else
        {
            //if (request().callType() == Request::FromDistribute &&
            //    DistributeRequestHooker::isAsyncWriteRequest(request().controller(), request().action()))
            //{
            //    std::cout << " a async request finish send, may not finished actually. Leave for async handler." << std::endl;
            //    return;
            //}
            std::cout << " sync request finished success. calltype: " << request().callType()
                << ", api: " << request().controller() << "_" << request().action() << std::endl;
            DistributeRequestHooker::get()->processLocalFinished(true);
        }
    }
}

bool Sf1Controller::callDistribute()
{
    if(request().callType() != Request::FromAPI)
        return false;
    if (MasterManagerBase::get()->isDistributed())
    {
        bool is_write_req = ReqLogMgr::isWriteRequest(request().controller(), request().action());
        if (is_write_req)
        {
            std::string reqdata;
            izenelib::driver::JsonWriter writer;
            writer.write(request().get(), reqdata);
            if(!MasterManagerBase::get()->pushWriteReq(reqdata))
                response().addError("push write request to queue failed.");
            if (ReqLogMgr::isAutoShardWriteReq(request().controller(), request().action()))
            {
                MasterManagerBase::get()->pushWriteReqToShard(reqdata, collectionName_);
            }
            return true;
        }
    }
    return false;
}

bool Sf1Controller::checkCollectionName()
{
    if (!collectionName_.empty())
        return true;

    response().addError(REQUIRE_COLLECTION_NAME);
    return false;
}

bool Sf1Controller::doCollectionCheck(std::string& error)
{
    return parseCollectionName(error) &&
           checkCollectionExist(error) &&
           checkCollectionAcl(error) &&
           checkCollectionHandler(error) &&
           checkCollectionService(error);
}

bool Sf1Controller::parseCollectionName(std::string& error)
{
    collectionName_ = asString(request()[Keys::collection]);

    if (!collectionName_.empty())
        return true;

    error = REQUIRE_COLLECTION_NAME;
    return false;
}

bool Sf1Controller::checkCollectionExist(std::string& error)
{
    if (SF1Config::get()->checkCollectionExist(collectionName_))
        return true;

    error = "Request failed, collection not found.";
    return false;
}

/**
 * User tokens has been get from request["header"]["acl_tokens"]. The collection
 * ACL rule is read from configuration file.
 *
 * If the request has set the collection but the collection is not allowed to
 * the user according to the access tokens, an error is returned.
 */
bool Sf1Controller::checkCollectionAcl(std::string& error)
{
    if (SF1Config::get()->checkCollectionAndACL(collectionName_, request().aclTokens()))
        return true;

    error = "Collection access denied";
    return false;
}

bool Sf1Controller::checkCollectionHandler(std::string& error)
{
    mutex_ = CollectionManager::get()->getCollectionMutex(collectionName_);
    mutex_->lock_shared();

    collectionHandler_ = CollectionManager::get()->findHandler(collectionName_);
    if (collectionHandler_)
        return true;

    error = "Collection handler not found";
    return false;
}

bool Sf1Controller::requireResourceProperty(
    const std::string& propName,
    std::string& propValue)
{
    const Value& resourceValue = request()[Keys::resource];

    if (! resourceValue.hasKey(propName))
    {
        response().addError("Require property \"" +
                            propName + "\" in request[resource].");
        return false;
    }

    propValue = asString(resourceValue[propName]);
    if (propValue.empty())
    {
        response().addError("Require non-empty value for property \"" +
                            propName + "\" in request[resource].");
        return false;
    }

    return true;
}

} // namespace sf1r

#include "Sf1WorkerController.h"
#include <process/controllers/CollectionHandler.h>
#include <common/XmlConfigParser.h>

#include <glog/logging.h>

namespace sf1r
{

Sf1WorkerController::Sf1WorkerController()
    : collectionHandler_(0)
    , mutex_(0)
{
}

Sf1WorkerController::~Sf1WorkerController()
{
    if(mutex_)
    {
        mutex_->unlock_shared();
    }
}

bool Sf1WorkerController::preprocess(msgpack::rpc::request& req, std::string& error)
{
    return parseCollectionName(req, error) &&
           checkCollectionHandler(error) &&
           checkWorker(error);
}

bool Sf1WorkerController::parseCollectionName(msgpack::rpc::request& req, std::string& error)
{
    try
    {
        msgpack::type::tuple<std::string> params;
        req.params().convert(&params);
        collectionName_ = params.get<0>();
    }
    catch (const msgpack::type_error& e)
    {
        error = "Failed to parse collection name, ";
        error += e.what();
        return false;
    }

    if (collectionName_.empty())
    {
        error = "Require non-empty collection name";
        return false;
    }

    return true;
}

bool Sf1WorkerController::checkCollectionHandler(std::string& error)
{
    mutex_ = CollectionManager::get()->getCollectionMutex(collectionName_);
    mutex_->lock_shared();

    collectionHandler_ = CollectionManager::get()->findHandler(collectionName_);
    if (collectionHandler_)
        return true;

    error = "Collection handler not found";
    return false;
}

} // namespace sf1r

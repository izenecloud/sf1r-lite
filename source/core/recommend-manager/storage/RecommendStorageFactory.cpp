#include "RecommendStorageFactory.h"
#include "LocalUserManager.h"
#include "RemoteUserManager.h"
#include "LocalPurchaseManager.h"
#include "RemotePurchaseManager.h"
#include "LocalVisitManager.h"
#include <configuration-manager/CassandraStorageConfig.h>
#include <log-manager/CassandraConnection.h>

#include <boost/filesystem.hpp>

namespace bfs = boost::filesystem;

namespace sf1r
{

RecommendStorageFactory::RecommendStorageFactory(
    const CassandraStorageConfig& cassandraConfig,
    const std::string& collection,
    const std::string& dataDir
)
    : cassandraConfig_(cassandraConfig)
    , cassandraClient_(NULL)
{
    if (cassandraConfig_.enable)
    {
        initRemoteStorage_(collection);
    }
    else
    {
        initLocalStorage_(dataDir);
    }
}

void RecommendStorageFactory::initRemoteStorage_(const std::string& collection)
{
    userColumnFamily_ = collection + "_users";
    purchaseColumnFamily_ = collection + "_purchase";

    const std::string& keyspace = cassandraConfig_.keyspace;
    cassandraClient_ = CassandraConnection::instance().getCassandraClient(keyspace);
    if (! cassandraClient_)
    {
        LOG(ERROR) << "failed to connect cassandra server, keyspace: " << keyspace;
    }
}

void RecommendStorageFactory::initLocalStorage_(const std::string& dataDir)
{
    bfs::path dataPath(dataDir);

    bfs::path userDir = dataPath / "user";
    bfs::create_directory(userDir);

    userPath_ = (userDir / "user.db").string();

    bfs::path eventDir = dataPath / "event";
    bfs::create_directory(eventDir);

    purchasePath_ = (eventDir / "purchase.db").string();
    visitItemPath_ = (eventDir / "visit_item.db").string();
    visitRecommendPath_ = (eventDir / "visit_recommend.db").string();
    visitSessionPath_ = (eventDir / "visit_session.db").string();
}

UserManager* RecommendStorageFactory::createUserManager() const
{
    if (cassandraConfig_.enable)
    {
        return new RemoteUserManager(cassandraConfig_.keyspace, userColumnFamily_, cassandraClient_);
    }
    else
    {
        return new LocalUserManager(userPath_);
    }
}

PurchaseManager* RecommendStorageFactory::createPurchaseManager() const
{
    if (cassandraConfig_.enable)
    {
        return new RemotePurchaseManager(cassandraConfig_.keyspace, purchaseColumnFamily_, cassandraClient_);
    }
    else
    {
        return new LocalPurchaseManager(purchasePath_);
    }
}

VisitManager* RecommendStorageFactory::createVisitManager() const
{
    if (cassandraConfig_.enable)
    {
        return NULL;
    }
    else
    {
        return new LocalVisitManager(visitItemPath_, visitRecommendPath_, visitSessionPath_);
    }
}

const std::string& RecommendStorageFactory::getUserColumnFamily() const
{
    return userColumnFamily_;
}

const std::string& RecommendStorageFactory::getPurchaseColumnFamily() const
{
    return purchaseColumnFamily_;
}

} // namespace sf1r

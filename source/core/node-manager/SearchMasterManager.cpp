#include "SearchMasterManager.h"
#include "SearchNodeManager.h"
#include "SuperNodeManager.h"
#include "ZooKeeperManager.h"

namespace sf1r
{

SearchMasterManager::SearchMasterManager()
{
    CLASSNAME = "SearchMasterManager";
}

bool SearchMasterManager::init()
{
    // initialize zookeeper client
    topologyPath_ = ZooKeeperNamespace::getSearchTopologyPath();
    serverParentPath_ = ZooKeeperNamespace::getSearchServerParentPath();
    serverPath_ = ZooKeeperNamespace::getSearchServerPath();
    zookeeper_ = ZooKeeperManager::get()->createClient(this);

    if (!zookeeper_)
        return false;

    sf1rTopology_ = SearchNodeManager::get()->getSf1rTopology();
    write_req_queue_ = ZooKeeperNamespace::getSearchWriteReqQueueNode();
    write_req_queue_parent_ = ZooKeeperNamespace::getSearchWriteReqQueueParent();
    return true;
}


}

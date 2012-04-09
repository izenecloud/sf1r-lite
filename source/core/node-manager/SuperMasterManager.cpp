#include "SuperMasterManager.h"

#include <boost/lexical_cast.hpp>

namespace sf1r
{

void SuperMasterManager::start()
{
    zookeeper_->connect(true);
    if (!zookeeper_ || !zookeeper_->isConnected())
    {
        LOG (ERROR) << "SuperMasterManager failed to connect ZooKeeper";
        return;
    }

    detectSearchMasters();
}

void SuperMasterManager::detectSearchMasters()
{
    boost::lock_guard<boost::mutex> lock(mutex_);

    replicaid_t replicaId = sf1rTopology_.curNode_.replicaId_;
    for (nodeid_t nodeId = 1; nodeId <= sf1rTopology_.nodeNum_; nodeId++)
    {
        std::string data;
        std::string nodePath = ZooKeeperNamespace::getSearchNodePath(replicaId, nodeId);
        if (zookeeper_->getZNodeData(nodePath, data, ZooKeeper::WATCH))
        {
            ZNode znode;
            znode.loadKvString(data);

            // if this sf1r node provides master server
            if (znode.hasKey(ZNode::KEY_MASTER_PORT))
            {
                if (masterMap_.find(nodeId) == masterMap_.end())
                {
                    boost::shared_ptr<Sf1rNode> sf1rNode(new Sf1rNode);

                    // get master info
                    sf1rNode->host_ = znode.getStrValue(ZNode::KEY_HOST);
                    sf1rNode->master_.name_ = znode.getStrValue(ZNode::KEY_MASTER_NAME);
                    try
                    {
                        sf1rNode->baPort_ =
                                boost::lexical_cast<uint32_t>(znode.getStrValue(ZNode::KEY_BA_PORT));
                    }
                    catch (std::exception& e)
                    {
                        LOG (ERROR) << "failed to convert baPort \"" << znode.getStrValue(ZNode::KEY_BA_PORT)
                                    << "\" got from master \"" << sf1rNode->master_.name_
                                    << "\" on node " << nodeId
                                    << " @" << sf1rNode->host_;
                        continue;
                    }

                    masterMap_[nodeId] = sf1rNode;

                    LOG (INFO) << "detected master, " << sf1rNode->host_ << ":" << sf1rNode->baPort_;
                }
            }
        }
    }

    LOG (INFO) << "detected " << masterMap_.size() << " master(s) in cluster.";
}

void SuperMasterManager::process(ZooKeeperEvent& zkEvent)
{
    // boost::lock_guard<boost::mutex> lock(mutex_);
}

void SuperMasterManager::detectRecommendMasters()
{
}

}

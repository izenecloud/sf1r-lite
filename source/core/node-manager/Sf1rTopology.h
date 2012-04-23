/**
 * @file Sf1rTopology.h
 * @author Zhongxia Li
 * @date Oct 20, 2011
 * @brief 
 */
#ifndef SF1R_TOPOLOGY_H_
#define SF1R_TOPOLOGY_H_

#include <stdint.h>
#include <string>
#include <vector>
#include <map>
#include <sstream>

namespace sf1r {

typedef uint32_t nodeid_t;
typedef uint32_t replicaid_t;
typedef uint32_t shardid_t;
typedef uint32_t port_t;


class MasterCollection
{
public:
    MasterCollection():isDistributive_(false) {}

public:
    std::string name_;
    bool isDistributive_;
    std::vector<shardid_t> shardList_;
};

class Sf1rNodeMaster
{
public:
    Sf1rNodeMaster():isEnabled_(false) {}

    bool checkCollection(const std::string& collection)
    {
        std::vector<MasterCollection>::iterator it;
        for (it = collectionList_.begin(); it != collectionList_.end(); it++)
        {
            if ((*it).name_ == collection)
                return true;
        }
        return false;
    }

    bool getShardidList(const std::string& collection, std::vector<shardid_t>& shardidList)
    {
        std::vector<MasterCollection>::iterator it;
        for (it = collectionList_.begin(); it != collectionList_.end(); it++)
        {
            if (it->name_ == collection)
            {
                shardidList = it->shardList_;
                return true;
            }
        }
        return false;
    }

    std::string toString()
    {
        std::stringstream ss;

        ss << "[Master]" << std::endl
           << "enabled: " << isEnabled_ << std::endl
           << "alias: " << name_ << std::endl
           << "port: " << port_ << std::endl
           << "total shards: " << totalShardNum_ << std::endl;

        std::vector<MasterCollection>::iterator it;
        for (it = collectionList_.begin(); it != collectionList_.end(); it++)
        {
            MasterCollection& col = *it;

            ss << "collection: " << col.name_ << ", "
               << (col.isDistributive_ ? "distributive" : "single node");

            if (col.isDistributive_)
            {
               ss << ", shards: ";
               for (size_t i = 0; i < col.shardList_.size(); i++)
               {
                   ss << col.shardList_[i] << ",";
               }
            }

            ss << std::endl;
        }

        return ss.str();
    }

public:
    bool isEnabled_;
    std::string name_;
    port_t port_;

    // Each shard resides on one of the Workers,
    // so it's also number of Workers, and shardid are used as workerid;
    shardid_t totalShardNum_;

    std::vector<MasterCollection> collectionList_;
};

class Sf1rNodeWorker
{
public:
    Sf1rNodeWorker()
        : isEnabled_(false)
        , isGood_(false)
    {}

    bool checkCollection(const std::string& collection)
    {
        std::vector<std::string>::iterator it;
        for (it = collectionList_.begin(); it != collectionList_.end(); it++)
        {
            if (*it == collection)
                return true;
        }
        return false;
    }

    std::string toString()
    {
        std::stringstream ss;

        ss << "[Worker]" << std::endl
           << "enabled: " << isEnabled_ << std::endl
           << "port: " << port_ << std::endl
           << "shardid (workerid):" << shardId_ << std::endl;

        for (size_t i = 0; i < collectionList_.size(); i++)
        {
            ss << "collection: " << collectionList_[i] << std::endl;
        }

        return ss.str();
    }

public:
    bool isEnabled_;
    bool isGood_;
    port_t port_;
    shardid_t shardId_; // id of shard resides on this worker.

    std::vector<std::string> collectionList_;
};

class Sf1rNode
{
public:
    std::string userName_;
    std::string host_;
    port_t baPort_;
    port_t dataPort_;

    replicaid_t replicaId_;
    nodeid_t nodeId_;

    Sf1rNodeMaster master_;
    Sf1rNodeWorker worker_;

public:
    std::string toString()
    {
        std::stringstream ss;

        ss << "[Sf1rNode]" << std::endl
           << "username: " << userName_ << std::endl
           << "host: " << host_ << std::endl
           << "BA port: " << baPort_ << std::endl
           << "data Port: " << dataPort_ << std::endl
           << "replica id: " << replicaId_ << std::endl
           << "node id: " << nodeId_ << std::endl;

        ss << master_.toString();
        ss << worker_.toString();

        return ss.str();
    }
};

class Sf1rTopology
{
public:
    enum TopologyType
    {
        TOPOLOGY_SEARCH,
        TOPOLOGY_RECOMMEND
    };

    std::string clusterId_;
    TopologyType type_;

    uint32_t nodeNum_;
    uint32_t replicaNum_;

    Sf1rNode curNode_;

public:
    Sf1rTopology()
        : type_(TOPOLOGY_SEARCH)
        , nodeNum_(0)
        , replicaNum_(1)
    {}

    void setType(std::string& type)
    {
        if (type == "search")
        {
            type_ = TOPOLOGY_SEARCH;
        }
        else if (type == "recommend")
        {
            type_ = TOPOLOGY_RECOMMEND;
        }
    }

    std::string toString()
    {
        std::stringstream ss;

        ss << "[Sf1rTopology]" << std::endl
           << "cluster id: " << clusterId_ << std::endl
           << "node number: " << nodeNum_ << std::endl
           << "replica number: " << replicaNum_ << std::endl;

        ss << curNode_.toString();

        return ss.str();
    }
};

}

#endif /* SF1R_TOPOLOGY_H_ */

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

    bool checkShard(shardid_t shardid) const
    {
        std::vector<shardid_t>::const_iterator it;
        for (it = shardList_.begin(); it != shardList_.end(); it++)
        {
            if (*it == shardid)
                return true;
        }
        return false;
    }

public:
    std::string name_;
    bool isDistributive_;
    std::vector<shardid_t> shardList_;
};

class WorkerServiceInfo
{
public:
    WorkerServiceInfo()
        :enabled_(false)
    {
    }
    bool enabled_;
    std::string serviceName_;
    std::vector<std::string> collectionList_;

    bool checkCollection(const std::string& collection) const
    {
        if (!enabled_)
            return false;
        std::vector<std::string>::const_iterator it;
        for (it = collectionList_.begin(); it != collectionList_.end(); it++)
        {
            if (*it == collection)
                return true;
        }
        return false;
    }
    std::string toString() const
    {
        std::stringstream ss;

        ss << "[Worker Service]" << std::endl
           << "service: " << serviceName_ << std::endl
           << "enabled_: " << enabled_ << std::endl;

        for (std::vector<std::string>::const_iterator it = collectionList_.begin();
           it != collectionList_.end(); ++it)
        {
            ss << "worker collection: " << *it << std::endl;
        }
        return ss.str();
    }
};

class MasterServiceInfo
{
public:
    bool enabled_;
    std::string serviceName_;
    std::string master_name_;
    std::vector<MasterCollection> collectionList_;

    MasterServiceInfo()
        :enabled_(false)
    {
    }
    bool checkCollection(const std::string& collection) const
    {
        if (!enabled_)
            return false;
        std::vector<MasterCollection>::const_iterator it;
        for (it = collectionList_.begin(); it != collectionList_.end(); it++)
        {
            if ((*it).name_ == collection)
                return true;
        }
        return false;
    }

    bool checkCollectionWorker(const std::string& collection, unsigned int shardid) const
    {
        if (!enabled_)
            return false;
        std::vector<MasterCollection>::const_iterator it;
        for (it = collectionList_.begin(); it != collectionList_.end(); it++)
        {
            const MasterCollection& coll = *it;
            if (coll.name_ == collection)
            {
                if (coll.checkShard(shardid))
                    return true;
                else
                    return false;
            }
        }
        return false;
    }

    bool getShardidList(const std::string& collection, std::vector<shardid_t>& shardidList) const
    {
        if (!enabled_)
            return false;
        std::vector<MasterCollection>::const_iterator it;
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
    
    std::string toString() const
    {
        std::stringstream ss;

        ss << "[MasterService]" << std::endl
           << "service name: " << serviceName_<< std::endl
           << "enabled_: " << enabled_ << std::endl
           << "master name: " << master_name_ << std::endl;

        std::vector<MasterCollection>::const_iterator it;
        for (it = collectionList_.begin(); it != collectionList_.end(); it++)
        {
            const MasterCollection& col = *it;

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

};

class Sf1rNodeMaster
{
public:
    typedef std::map<std::string, MasterServiceInfo> MasterServiceMapT;

    Sf1rNodeMaster(){}

    bool checkCollection(const std::string& service, const std::string& collection) const
    {
        MasterServiceMapT::const_iterator cit = masterServices_.find(service);
        if (cit == masterServices_.end())
            return false;
        return cit->second.checkCollection(collection);
    }

    bool checkCollectionWorker(const std::string& service, const std::string& collection, unsigned int shardid) const
    {
        MasterServiceMapT::const_iterator cit = masterServices_.find(service);
        if (cit == masterServices_.end())
            return false;
        return cit->second.checkCollectionWorker(collection, shardid);
    }

    bool getShardidList(const std::string& service, const std::string& collection, std::vector<shardid_t>& shardidList) const
    {
        MasterServiceMapT::const_iterator cit = masterServices_.find(service);
        if (cit == masterServices_.end())
            return false;
        return cit->second.getShardidList(collection, shardidList);
    }

    std::string toString() const
    {
        std::stringstream ss;

        ss << "[Master]" << std::endl
           << "port: " << port_ << std::endl;

        MasterServiceMapT::const_iterator it;
        for (it = masterServices_.begin(); it != masterServices_.end(); it++)
        {
            ss << it->second.toString() << std::endl;
        }

        return ss.str();
    }

    bool hasAnyService()
    {
        return !masterServices_.empty();
    }

public:
    port_t port_;
    // Each shard resides on one of the Workers,
    // so it's also number of Workers, and shardid are used as workerid;
    //uint32_t totalShardNum_;
    MasterServiceMapT masterServices_;
};

class Sf1rNodeWorker
{
public:
    typedef std::map<std::string, WorkerServiceInfo>  WorkerServiceMapT;
    Sf1rNodeWorker()
        :isGood_(false)
    {}

    bool checkCollection(const std::string& service, const std::string& collection)
    {
        WorkerServiceMapT::const_iterator cit = workerServices_.find(service);
        if (cit == workerServices_.end())
            return false;
        return cit->second.checkCollection(collection);
    }

    std::string toString()
    {
        std::stringstream ss;

        ss << "[Worker]" << std::endl
           << "port: " << port_ << std::endl;

        for (WorkerServiceMapT::const_iterator it = workerServices_.begin();
           it != workerServices_.end(); ++it)
        {
            ss << "worker services : " << it->first << ", " << it->second.toString() << std::endl;
        }

        return ss.str();
    }

    bool hasAnyService()
    {
        return !workerServices_.empty();
    }

public:
    bool isGood_;
    port_t port_;

    WorkerServiceMapT workerServices_;
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
    std::string clusterId_;
    // the node number in each replica
    uint32_t nodeNum_;
    uint32_t replicaNum_;

    Sf1rNode curNode_;

    enum DistributeServiceType
    {
        SearchService,
        RecommendService,
    };

public:
    Sf1rTopology()
        :nodeNum_(0)
        , replicaNum_(1)
    {}

    static std::string getServiceName(DistributeServiceType type)
    {
        if (type == SearchService)
        {
            return "search";
        }
        else if (type == RecommendService)
        {
            return "recommend";
        }
        return "";
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

/**
 * @file /sf1r-engine/source/core/node-manager/SuperMasterManager.h
 * @author Zhongxia Li
 * @date Apr 1, 2012
 * @brief Coordination between masters
 */

#ifndef SUPER_MASTER_MANAGER_H_
#define SUPER_MASTER_MANAGER_H_

#include "ZooKeeperNamespace.h"
#include "ZooKeeperManager.h"

#include "Sf1rTopology.h"

#include <util/singleton.h>

namespace sf1r
{

/**
 * Provide interfaces according coordination strategy
 */
class SuperMasterManager : public ZooKeeperEventHandler
{
public:
    typedef std::map<nodeid_t, boost::shared_ptr<Sf1rNode> > MasterMapT;

public:
    static SuperMasterManager* get()
    {
        return izenelib::util::Singleton<SuperMasterManager>::get();
    }

    void init(const Sf1rTopology& sf1rTopology)
    {
        sf1rTopology_ = sf1rTopology;

        zookeeper_ = ZooKeeperManager::get()->createClient(this);
    }

    void start();

public:
    virtual void process(ZooKeeperEvent& zkEvent);

private:
    void detectSearchMasters();

    // todo
    void detectRecommendMasters();

private:
    ZooKeeperClientPtr zookeeper_;

    Sf1rTopology sf1rTopology_;

    MasterMapT masterMap_;
    boost::mutex mutex_;
};

}

#endif /* SUPER_MASTER_MANAGER_H_ */

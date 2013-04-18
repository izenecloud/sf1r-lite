/**
 * @file RecommendNodeManager.h
 * @author Zhongxia Li
 * @date Dec 8, 2011
 * @brief 
 */
#ifndef DISTRIBUTE_RECOMMEND_SERVICE_H
#define DISTRIBUTE_RECOMMEND_SERVICE_H

#include "IDistributeService.h"
#include "SearchMasterAddressFinder.h"
#include "Sf1rTopology.h"
#include <aggregator-manager/MasterServerConnector.h>

namespace sf1r
{

class DistributeRecommendService : public IDistributeService, public SearchMasterAddressFinder
{
public:
    DistributeRecommendService();

    std::string getServiceName()
    {
        return Sf1rTopology::getServiceName(Sf1rTopology::RecommendService);
    }

    bool initWorker()
    {
        return true;
    }
    bool initMaster()
    {
        MasterServerConnector::get()->init(this);
        return true;
    }
    virtual bool findSearchMasterAddress(std::string& host, uint32_t& port);
};

}

#endif /* DISTRIBUTE_RECOMMEND_SERVICEWORKER_H*/

/**
 * @file SearchNodeManager.h
 * @author Zhongxia Li
 * @date Dec 8, 2011
 * @brief 
 */
#ifndef DISTRIBUTE_SEARCH_SEVICE_H
#define DISTRIBUTE_SEARCH_SEVICE_H

#include "IDistributeService.h"

namespace sf1r
{

class DistributeSearchService: public IDistributeService
{
public:
    DistributeSearchService();
    std::string getServiceName();
    bool initWorker()
    {
        return true;
    }
    bool initMaster()
    {
        return true;
    }
};

}

#endif /* DISTRIBUTE_SEARCH_SEVICE_H*/

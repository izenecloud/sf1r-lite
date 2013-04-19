#ifndef NODE_MANAGER_IDISTRIBUTE_SERVICE_H
#define NODE_MANAGER_IDISTRIBUTE_SERVICE_H

#include <string>

namespace sf1r
{

class IDistributeService
{
public:
    virtual ~IDistributeService(){}
    virtual std::string getServiceName() = 0;
    virtual bool initWorker() = 0;
    virtual bool initMaster() = 0;
};

}

#endif

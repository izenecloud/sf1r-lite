#include "DistributeRecommendService.h"
#include "DistributeSearchService.h"
#include "MasterManagerBase.h"

namespace sf1r
{

DistributeRecommendService::DistributeRecommendService()
{
}

bool DistributeRecommendService::findSearchMasterAddress(std::string& host, uint32_t& port)
{
    static DistributeSearchService search_service;
    return MasterManagerBase::get()->findServiceMasterAddress(search_service.getServiceName(), host, port);
}

}



#include "DistributeSearchService.h"
#include "Sf1rTopology.h"

namespace sf1r
{

DistributeSearchService::DistributeSearchService()
{
}

std::string DistributeSearchService::getServiceName()
{
    return Sf1rTopology::getServiceName(Sf1rTopology::SearchService);
}



}

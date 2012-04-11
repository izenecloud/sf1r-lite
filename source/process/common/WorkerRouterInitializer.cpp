#include "WorkerRouterInitializer.h"
#include <process/distribute/SearchWorkerController.h>
#include <process/distribute/IndexWorkerController.h>
#include <process/distribute/GetRecommendWorkerController.h>
#include <process/distribute/UpdateRecommendWorkerController.h>

#include <net/aggregator/WorkerRouter.h>
#include <net/aggregator/WorkerController.h>
#include <glog/logging.h>

namespace sf1r
{

WorkerRouterInitializer::WorkerRouterInitializer()
{
    controllers_.push_back(new SearchWorkerController);
    controllers_.push_back(new IndexWorkerController);
    controllers_.push_back(new GetRecommendWorkerController);
    controllers_.push_back(new UpdateRecommendWorkerController);
}

WorkerRouterInitializer::~WorkerRouterInitializer()
{
    for (WorkerControllerList::iterator it = controllers_.begin();
        it != controllers_.end(); ++it)
    {
        delete *it;
    }
}

bool WorkerRouterInitializer::initRouter(net::aggregator::WorkerRouter& router)
{
    for (WorkerControllerList::iterator it = controllers_.begin();
        it != controllers_.end(); ++it)
    {
        if (! (*it)->addWorkerHandler(router))
        {
            LOG(ERROR) << "failed to initialize WorkerRouter";
            return false;
        }
    }

    return true;
}

} // namespace sf1r

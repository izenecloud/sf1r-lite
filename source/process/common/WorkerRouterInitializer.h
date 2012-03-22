/**
 * @file WorkerRouterInitializer.h
 * @brief initialize WorkerRouter
 * @author Jun Jiang
 * @date 2012-03-22
 */

#ifndef WORKER_ROUTER_INITIALIZER_H
#define WORKER_ROUTER_INITIALIZER_H

#include <vector>

namespace net{
namespace aggregator{

class WorkerRouter;
class WorkerController;

}}

namespace sf1r
{

class WorkerRouterInitializer
{
public:
    WorkerRouterInitializer();

    ~WorkerRouterInitializer();

    bool initRouter(net::aggregator::WorkerRouter& router);

private:
    typedef std::vector<net::aggregator::WorkerController*> WorkerControllerList;
    WorkerControllerList controllers_;
};

} // namespace sf1r

#endif // WORKER_ROUTER_INITIALIZER_H

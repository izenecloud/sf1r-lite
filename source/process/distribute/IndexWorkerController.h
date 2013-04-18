/**
 * @file IndexWorkerController.h
 * @brief handle index request to worker server
 * @author Zhongxia Li, Jun Jiang
 * @date 2012-03-22
 */

#ifndef INDEX_WORKER_CONTROLLER_H_
#define INDEX_WORKER_CONTROLLER_H_

#include "Sf1WorkerController.h"
#include <aggregator-manager/IndexWorker.h>

#include <boost/shared_ptr.hpp>

namespace sf1r
{

class IndexWorkerController : public Sf1WorkerController
{
public:
    virtual bool addWorkerHandler(net::aggregator::WorkerRouter& router)
    {
        ADD_WORKER_HANDLER_BEGIN(IndexWorkerController, router)

        ADD_WORKER_HANDLER(index)
        ADD_WORKER_HANDLER(HookDistributeRequestForIndex)

        ADD_WORKER_HANDLER_END()
    }

    WORKER_CONTROLLER_METHOD_2(index, indexWorker_->index, unsigned int, bool)
    WORKER_CONTROLLER_METHOD_3(HookDistributeRequestForIndex, indexWorker_->HookDistributeRequestForIndex, int, std::string, bool)

protected:
    virtual bool checkWorker(std::string& error);

private:
    boost::shared_ptr<IndexWorker> indexWorker_;
};

} // namespace sf1r

#endif // INDEX_WORKER_CONTROLLER_H_

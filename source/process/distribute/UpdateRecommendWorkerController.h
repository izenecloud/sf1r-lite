/**
 * @file UpdateRecommendWorkerController.h
 * @brief handle request to udpate recommend matrix in worker server
 * @author Jun Jiang
 * @date 2012-04-11
 */

#ifndef UPDATE_RECOMMEND_WORKER_CONTROLLER_H_
#define UPDATE_RECOMMEND_WORKER_CONTROLLER_H_

#include "Sf1WorkerController.h"
#include <aggregator-manager/UpdateRecommendWorker.h>

namespace sf1r
{

class UpdateRecommendWorkerController : public Sf1WorkerController
{
public:
    UpdateRecommendWorkerController() : worker_(NULL) {}

    virtual bool addWorkerHandler(net::aggregator::WorkerRouter& router)
    {
        ADD_WORKER_HANDLER_BEGIN(UpdateRecommendWorkerController, router)

        ADD_WORKER_HANDLER(updatePurchaseMatrix)
        ADD_WORKER_HANDLER(updatePurchaseCoVisitMatrix)
        ADD_WORKER_HANDLER(buildPurchaseSimMatrix)
        ADD_WORKER_HANDLER(updateVisitMatrix)
        ADD_WORKER_HANDLER(flushRecommendMatrix)
        ADD_WORKER_HANDLER(HookDistributeRequestForUpdateRec)

        ADD_WORKER_HANDLER_END()
    }

    WORKER_CONTROLLER_METHOD_3(
        updatePurchaseMatrix,
        worker_->updatePurchaseMatrix,
        std::list<itemid_t>,
        std::list<itemid_t>,
        bool
    )

    WORKER_CONTROLLER_METHOD_3(
        updatePurchaseCoVisitMatrix,
        worker_->updatePurchaseCoVisitMatrix,
        std::list<itemid_t>,
        std::list<itemid_t>,
        bool
    )

    WORKER_CONTROLLER_METHOD_1(
        buildPurchaseSimMatrix,
        worker_->buildPurchaseSimMatrix,
        bool
    )

    WORKER_CONTROLLER_METHOD_3(
        updateVisitMatrix,
        worker_->updateVisitMatrix,
        std::list<itemid_t>,
        std::list<itemid_t>,
        bool
    )

    WORKER_CONTROLLER_METHOD_1(
        flushRecommendMatrix,
        worker_->flushRecommendMatrix,
        bool
    )

    WORKER_CONTROLLER_METHOD_3(HookDistributeRequestForUpdateRec,
        worker_->HookDistributeRequestForUpdateRec, int, std::string, bool)

protected:
    virtual bool checkWorker(std::string& error);

private:
    UpdateRecommendWorker* worker_;
};

} // namespace sf1r

#endif // UPDATE_RECOMMEND_WORKER_CONTROLLER_H_

/**
 * @file GetRecommendWorkerController.h
 * @brief handle request to get recommend result in worker server
 * @author Jun Jiang
 * @date 2012-04-11
 */

#ifndef GET_RECOMMEND_WORKER_CONTROLLER_H_
#define GET_RECOMMEND_WORKER_CONTROLLER_H_

#include "Sf1WorkerController.h"
#include <aggregator-manager/GetRecommendWorker.h>

namespace sf1r
{

class GetRecommendWorkerController : public Sf1WorkerController
{
public:
    GetRecommendWorkerController() : worker_(NULL) {}

    virtual bool addWorkerHandler(net::aggregator::WorkerRouter& router)
    {
        ADD_WORKER_HANDLER_BEGIN(GetRecommendWorkerController, router)

        ADD_WORKER_HANDLER(recommendPurchase)
        ADD_WORKER_HANDLER(recommendPurchaseFromWeight)
        ADD_WORKER_HANDLER(recommendVisit)

        ADD_WORKER_HANDLER_END()
    }

    WORKER_CONTROLLER_METHOD_2(
        recommendPurchase,
        worker_->recommendPurchase,
        RecommendInputParam,
        idmlib::recommender::RecommendItemVec
    )

    WORKER_CONTROLLER_METHOD_2(
        recommendPurchaseFromWeight,
        worker_->recommendPurchaseFromWeight,
        RecommendInputParam,
        idmlib::recommender::RecommendItemVec
    )

    WORKER_CONTROLLER_METHOD_2(
        recommendVisit,
        worker_->recommendVisit,
        RecommendInputParam,
        idmlib::recommender::RecommendItemVec
    )

protected:
    virtual bool checkWorker(std::string& error);

private:
    GetRecommendWorker* worker_;
};

} // namespace sf1r

#endif // GET_RECOMMEND_WORKER_CONTROLLER_H_

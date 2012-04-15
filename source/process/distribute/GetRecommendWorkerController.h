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

        ADD_WORKER_HANDLER(recommendVisit)
        ADD_WORKER_HANDLER(getCandidateSet)
        ADD_WORKER_HANDLER(recommendFromCandidateSet)
        ADD_WORKER_HANDLER(getCandidateSetFromWeight)
        ADD_WORKER_HANDLER(recommendFromCandidateSetWeight)

        ADD_WORKER_HANDLER_END()
    }

    WORKER_CONTROLLER_METHOD_2(
        recommendVisit,
        worker_->recommendVisit,
        RecommendInputParam,
        idmlib::recommender::RecommendItemVec
    )

    WORKER_CONTROLLER_METHOD_2(
        getCandidateSet,
        worker_->getCandidateSet,
        std::vector<itemid_t>,
        std::set<itemid_t>
    )

    WORKER_CONTROLLER_METHOD_3(
        recommendFromCandidateSet,
        worker_->recommendFromCandidateSet,
        RecommendInputParam,
        std::set<itemid_t>,
        idmlib::recommender::RecommendItemVec
    )

    WORKER_CONTROLLER_METHOD_2(
        getCandidateSetFromWeight,
        worker_->getCandidateSetFromWeight,
        ItemCFManager::ItemWeightMap,
        std::set<itemid_t>
    )

    WORKER_CONTROLLER_METHOD_3(
        recommendFromCandidateSetWeight,
        worker_->recommendFromCandidateSetWeight,
        RecommendInputParam,
        std::set<itemid_t>,
        idmlib::recommender::RecommendItemVec
    )

protected:
    virtual bool checkWorker(std::string& error);

private:
    GetRecommendWorker* worker_;
};

} // namespace sf1r

#endif // GET_RECOMMEND_WORKER_CONTROLLER_H_

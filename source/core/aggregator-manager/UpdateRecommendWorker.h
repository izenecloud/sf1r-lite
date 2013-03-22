/**
 * @file UpdateRecommendWorker.h
 * @brief update recommendation matrix in worker
 * @author Jun Jiang
 * @date 2012-04-09
 */

#ifndef SF1R_UPDATE_RECOMMEND_WORKER_H
#define SF1R_UPDATE_RECOMMEND_WORKER_H

#include "UpdateRecommendBase.h"
#include <net/aggregator/BindCallProxyBase.h>
#include <common/JobScheduler.h>
#include <util/osgi/IService.h>

namespace sf1r
{

class UpdateRecommendWorker : public UpdateRecommendBase
                            , public net::aggregator::BindCallProxyBase<UpdateRecommendWorker>
                            , public ::izenelib::osgi::IService
{
public:
    UpdateRecommendWorker(
        const std::string& collection,
        ItemCFManager& itemCFManager,
        CoVisitManager& coVisitManager
    );

    virtual bool bindCallProxy(CallProxyType& proxy)
    {
        BIND_CALL_PROXY_BEGIN(UpdateRecommendWorker, proxy)
        BIND_CALL_PROXY_3(
            updatePurchaseMatrix,
            std::list<itemid_t>,
            std::list<itemid_t>,
            bool
        )
        BIND_CALL_PROXY_3(
            updatePurchaseCoVisitMatrix,
            std::list<itemid_t>,
            std::list<itemid_t>,
            bool
        )
        BIND_CALL_PROXY_1(
            buildPurchaseSimMatrix,
            bool
        )
        BIND_CALL_PROXY_3(
            updateVisitMatrix,
            std::list<itemid_t>,
            std::list<itemid_t>,
            bool
        )
        BIND_CALL_PROXY_1(
            flushRecommendMatrix,
            bool
        )
        BIND_CALL_PROXY_3(HookDistributeRequestForUpdateRec, int, std::string, bool)
        BIND_CALL_PROXY_END()
    }

    virtual void updatePurchaseMatrix(
        const std::list<itemid_t>& oldItems,
        const std::list<itemid_t>& newItems,
        bool& result
    );

    virtual void updatePurchaseCoVisitMatrix(
        const std::list<itemid_t>& oldItems,
        const std::list<itemid_t>& newItems,
        bool& result
    );

    virtual void buildPurchaseSimMatrix(bool& result);

    virtual void updateVisitMatrix(
        const std::list<itemid_t>& oldItems,
        const std::list<itemid_t>& newItems,
        bool& result
    );

    virtual void flushRecommendMatrix(bool& result);

    void HookDistributeRequestForUpdateRec(int hooktype, const std::string& reqdata, bool& result);

    const ItemCFManager& itemCFManager() const { return itemCFManager_; }

private:
    bool updatePurchaseMatrixFunc(int calltype);
    bool updatePurchaseCoVisitMatrixFunc(int calltype);
    bool updateVisitMatrixFunc(int calltype);
    bool buildPurchaseSimMatrixFunc(int calltype);

    void flushImpl_();

private:
    ItemCFManager& itemCFManager_;
    CoVisitManager& coVisitManager_;

    JobScheduler jobScheduler_;
};

} // namespace sf1r

#endif // SF1R_UPDATE_RECOMMEND_WORKER_H

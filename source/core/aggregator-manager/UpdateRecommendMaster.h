/**
 * @file UpdateRecommendMaster.h
 * @brief the master sends request to update recommendation matrix in workers
 * @author Jun Jiang
 * @date 2012-04-10
 */

#ifndef SF1R_UPDATE_RECOMMEND_MASTER_H
#define SF1R_UPDATE_RECOMMEND_MASTER_H

#include "UpdateRecommendBase.h"
#include "UpdateRecommendAggregator.h"

#include <string>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

namespace sf1r
{

class UpdateRecommendWorker;

class UpdateRecommendMaster : public UpdateRecommendBase
{
public:
    /**
     * @param collection collection name
     * @param localWorker it could be NULL if no local worker exists
     */
    UpdateRecommendMaster(
        const std::string& collection,
        UpdateRecommendWorker* localWorker
    );

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

private:
    const std::string collection_;
    boost::scoped_ptr<UpdateRecommendMerger> merger_;
    boost::shared_ptr<UpdateRecommendAggregator> aggregator_;
};

} // namespace sf1r

#endif // SF1R_UPDATE_RECOMMEND_MASTER_H

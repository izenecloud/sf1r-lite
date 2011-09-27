#ifndef PROCESS_CONTROLLERS_COLLECTIONMININGCONTROLLER_H
#define PROCESS_CONTROLLERS_COLLECTIONMININGCONTROLLER_H
/**
 * @file process/controllers/CollectionMiningController.h
 * @author Jia Guo
 */
#include "Sf1Controller.h"
#include "CollectionHandler.h"
#include <bundles/mining/MiningSearchService.h>
#include <bundles/index/IndexTaskService.h>

namespace sf1r
{

class CollectionMiningController : public Sf1Controller
{
public:
    
    bool preprocess()
    {
        if (!Sf1Controller::preprocess())
        {
            return false;
        }
        
        if(!GetMiningManager()) return false;

        return true;
    }

    boost::shared_ptr<MiningManager> GetMiningManager()
    {
        MiningSearchService* service = collectionHandler_->miningSearchService_;
        return service->GetMiningManager();
    }

    std::string GetProductSourceField()
    {
        return (collectionHandler_->miningSchema_).product_source_property;
    }

    uint32_t GetDocNum()
    {
        IndexTaskService* service = collectionHandler_->indexTaskService_;
        return service->getDocNum();
    }

};


} // namespace sf1r

#endif 



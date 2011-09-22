#ifndef PROCESS_CONTROLLERS_COLLECTIONMININGCONTROLLER_H
#define PROCESS_CONTROLLERS_COLLECTIONMININGCONTROLLER_H
/**
 * @file process/controllers/CollectionMiningController.h
 * @author Jia Guo
 */
#include "Sf1Controller.h"
#include "CollectionHandler.h"
#include <bundles/mining/MiningSearchService.h>

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

};


} // namespace sf1r

#endif 



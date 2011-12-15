/**
 * @file RecommendNodeManager.h
 * @author Zhongxia Li
 * @date Dec 8, 2011
 * @brief 
 */
#ifndef RECOMMEND_NODE_MANAGER_H_
#define RECOMMEND_NODE_MANAGER_H_

#include "NodeManagerBase.h"
#include "RecommendMasterManager.h"

#include <util/singleton.h>

namespace sf1r
{

class RecommendNodeManager : public NodeManagerBase
{
public:
    RecommendNodeManager()
    {
        CLASSNAME = "[RecommendNodeManager]";
    }

    static RecommendNodeManager* get()
    {
        return izenelib::util::Singleton<RecommendNodeManager>::get();
    }

protected:
    virtual void startMasterManager()
    {
        //RecommendMasterManagerSingleton::get()->start();
    }

    virtual void stopMasterManager()
    {
        //RecommendMasterManagerSingleton::get()->stop();
    }
};

}

#endif /* RECOMMEND_NODE_MANAGER_H_ */

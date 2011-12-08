/**
 * @file RecommendNodeManager.h
 * @author Zhongxia Li
 * @date Dec 8, 2011
 * @brief 
 */
#ifndef RECOMMEND_NODE_MANAGER_H_
#define RECOMMEND_NODE_MANAGER_H_

#include "NodeManager.h"
#include "RecommendMasterManager.h"

#include <util/singleton.h>

namespace sf1r
{

class RecommendNodeManager : public NodeManager
{
public:
    RecommendNodeManager()
    {
        CLASSNAME = "[RecommendNodeManager]";
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

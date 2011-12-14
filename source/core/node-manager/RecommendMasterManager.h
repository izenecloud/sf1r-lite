/**
 * @file RecommendMasterManager.h
 * @author Zhongxia Li
 * @date Dec 8, 2011
 * @brief 
 */
#ifndef RECOMMEND_MASTER_MANAGER_H_
#define RECOMMEND_MASTER_MANAGER_H_

#include "MasterManagerBase.h"


namespace sf1r
{

class RecommendMasterManager : public MasterManagerBase
{
public:
    RecommendMasterManager();

    virtual bool init();
};

}

#endif /* RECOMMEND_MASTER_MANAGER_H_ */

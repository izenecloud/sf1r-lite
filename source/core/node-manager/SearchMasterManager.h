/**
 * @file SearchMasterManager.h
 * @author Zhongxia Li
 * @date Dec 8, 2011
 * @brief 
 */
#ifndef SEARCH_MASTER_MANAGER_H_
#define SEARCH_MASTER_MANAGER_H_

#include "MasterManager.h"

namespace sf1r
{

class SearchMasterManager : public MasterManager
{
public:
    SearchMasterManager();

    virtual void init();
};

typedef izenelib::util::Singleton<SearchMasterManager> SearchMasterManagerSingleton;

}

#endif /* SEARCH_MASTER_MANAGER_H_ */

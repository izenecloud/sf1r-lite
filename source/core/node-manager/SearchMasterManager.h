/**
 * @file SearchMasterManager.h
 * @author Zhongxia Li
 * @date Dec 8, 2011
 * @brief 
 */
#ifndef SEARCH_MASTER_MANAGER_H_
#define SEARCH_MASTER_MANAGER_H_

#include "MasterManager.h"

#include <util/singleton.h>

namespace sf1r
{

class SearchMasterManager : public MasterManager
{
public:
    SearchMasterManager();

    static SearchMasterManager* get()
    {
        return izenelib::util::Singleton<SearchMasterManager>::get();
    }

    virtual bool init();
};


}

#endif /* SEARCH_MASTER_MANAGER_H_ */

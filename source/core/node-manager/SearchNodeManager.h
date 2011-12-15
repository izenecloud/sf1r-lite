/**
 * @file SearchNodeManager.h
 * @author Zhongxia Li
 * @date Dec 8, 2011
 * @brief 
 */
#ifndef SEARCH_NODE_MANAGER_H_
#define SEARCH_NODE_MANAGER_H_

#include "NodeManagerBase.h"
#include "SearchMasterManager.h"

#include <util/singleton.h>

namespace sf1r
{

class SearchNodeManager : public NodeManagerBase
{
public:
    SearchNodeManager()
    {
        CLASSNAME = "[SearchNodeManager]";
    }

    static SearchNodeManager* get()
    {
        return izenelib::util::Singleton<SearchNodeManager>::get();
    }

protected:
    virtual void startMasterManager()
    {
        SearchMasterManager::get()->start();
    }

    virtual void stopMasterManager()
    {
        SearchMasterManager::get()->stop();
    }
};


}

#endif /* SEARCH_NODE_MANAGER_H_ */

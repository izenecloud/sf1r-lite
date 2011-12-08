/**
 * @file SearchNodeManager.h
 * @author Zhongxia Li
 * @date Dec 8, 2011
 * @brief 
 */
#ifndef SEARCH_NODE_MANAGER_H_
#define SEARCH_NODE_MANAGER_H_

#include "NodeManager.h"
#include "SearchMasterManager.h"

#include <util/singleton.h>

namespace sf1r
{

class SearchNodeManager : public NodeManager
{
public:
    SearchNodeManager()
    {
        CLASSNAME = "[SearchNodeManager]";
    }

protected:
    virtual void startMasterManager()
    {
        SearchMasterManagerSingleton::get()->start();
    }

    virtual void stopMasterManager()
    {
        SearchMasterManagerSingleton::get()->stop();
    }
};

typedef izenelib::util::Singleton<SearchNodeManager> SearchNodeManagerSingleton;

}

#endif /* SEARCH_NODE_MANAGER_H_ */

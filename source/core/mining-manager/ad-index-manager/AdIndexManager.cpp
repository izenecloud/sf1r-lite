/*
 *  AdIndexManager.cpp
 */

#include "AdIndexManager.h"

namespace sf1r
{
AdIndexManager::AdIndexManager(
        const std::string& path,
        boost::shared_ptr<DocumentManager> dm)
    :indexPath_(path), documentManager_(dm)
{
}
AdIndexManager::~AdIndexManager()
{
    if(adMiningTask_)
        delete adMiningTask_;
}
bool AdIndexManager::buildMiningTask()
{
    adMiningTask_ = new AdMiningTask(indexPath_, documentManager_);
    return adMiningTask_ ? true : false;
}

bool search(const std::vector<std::pair<std::string, std::string> >& info,
        boost::unordered_set<std::string>& docids)
{
    return true;
}

} //namespace sf1r

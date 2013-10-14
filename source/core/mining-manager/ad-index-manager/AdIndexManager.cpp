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

bool AdIndexManager::search(const std::vector<std::pair<std::string, std::string> >& info,
        std::vector<docid_t>& docids)
{
    boost::unordered_set<uint32_t> dnfIDs;

    adMiningTask_->retrieve(info, dnfIDs);

    for(boost::unordered_set<uint32_t>::iterator it = dnfIDs.begin();
            it != dnfIDs.end(); it++ )
    {
        docids.push_back(*it);
    }
    return true;
}

} //namespace sf1r

/*
 *  AdIndexManager.cpp
 */

#include "AdIndexManager.h"

namespace sf1r
{

AdIndexManager::AdIndexManager(
        const std::string& path,
        boost::shared_ptr<DocumentManager>& dm)
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
        std::vector<docid_t>& docids,
        std::vector<float>& topKScoreRankList,
        std::size_t& totalCount)
{
    boost::unordered_set<uint32_t> dnfIDs;

    adMiningTask_->retrieve(info, dnfIDs);

    Document doc;
    for(boost::unordered_set<uint32_t>::iterator it = dnfIDs.begin();
            it != dnfIDs.end(); it++ )
    {
        if(documentManager_->getDocument(*it, doc) && !documentManager_->isDeleted(*it))
        {
            docids.push_back(*it);
            std::string price;
            std::string bidMode;
            doc.getProperty("Price", price);
            doc.getProperty("BidMode", bidMode);
            if(bidMode == "CPM")
            {
                topKScoreRankList.push_back(boost::lexical_cast<float>(price));
            }
            else if(bidMode == "CPC")
            {
                //TODO
                // calculate CTR
                // calcutate eCPM
                topKScoreRankList.push_back(boost::lexical_cast<float>(price));
            }
        }
    }
    totalCount = docids.size();
    return true;
}

} //namespace sf1r

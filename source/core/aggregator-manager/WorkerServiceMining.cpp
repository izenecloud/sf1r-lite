#include "WorkerService.h"

#include <mining-manager/MiningManager.h>

namespace sf1r
{

bool WorkerService::getSimilarDocIdList(uint64_t& documentId, uint32_t& maxNum, SimilarDocIdListType& result)
{
    // todo get all similar docs in global space?
    //return miningManager_->getSimilarDocIdList(documentId, maxNum, result);

    std::pair<sf1r::workerid_t, sf1r::docid_t> wd = net::aggregator::Util::GetWorkerAndDocId(documentId);
    sf1r::workerid_t workerId = wd.first;
    sf1r::docid_t docId = wd.second;

    std::vector<std::pair<uint32_t, float> > simDocList;
    bool ret = miningManager_->getSimilarDocIdList(docId, maxNum, simDocList); //get similar docs in current machine.

    result.resize(simDocList.size());
    for (size_t i = 0; i < simDocList.size(); i ++)
    {
        uint64_t wdocid = net::aggregator::Util::GetWDocId(workerId, simDocList[i].first);
        result[i] = std::make_pair(wdocid, simDocList[i].second);
    }

    return ret;
}

bool WorkerService::clickGroupLabel(const ClickGroupLabelActionItem& actionItem, bool& ret)
{
    ret = miningManager_->clickGroupLabel(
            actionItem.queryString_,
            actionItem.propName_,
            actionItem.groupPath_);

    return ret;
}

bool WorkerService::visitDoc(const uint32_t& docId, bool& ret)
{
    ret = miningManager_->visitDoc(docId);
    return ret;
}

}

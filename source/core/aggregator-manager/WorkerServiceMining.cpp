#include "WorkerService.h"

#include <mining-manager/MiningManager.h>

namespace sf1r
{

bool WorkerService::getSimilarDocIdList(uint32_t& documentId, uint32_t& maxNum, SimilarDocIdListType& result)
{
    return miningManager_->getSimilarDocIdList(documentId, maxNum, result);
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

#include "WorkerService.h"

#include <mining-manager/MiningManager.h>

namespace sf1r
{

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

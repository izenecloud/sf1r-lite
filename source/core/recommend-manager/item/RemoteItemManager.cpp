#include "RemoteItemManager.h"
#include "ItemIdGenerator.h"
#include <query-manager/ActionItem.h>
#include <common/ResultType.h>
#include <aggregator-manager/MasterServerConnector.h>

#include <glog/logging.h>
#include <stdexcept>

namespace sf1r
{

RemoteItemManager::RemoteItemManager(
    const std::string& collection,
    ItemIdGenerator* itemIdGenerator
)
    : RemoteItemManagerBase(collection, itemIdGenerator)
    , masterServerConnector_(MasterServerConnector::get())
{
}

bool RemoteItemManager::sendRequest_(
    const GetDocumentsByIdsActionItem& request,
    RawTextResultFromSIA& response
)
{
    try
    {
        masterServerConnector_->syncCall(MasterServerConnector::Method_getDocumentsByIds_,
                                         request, response);
    }
    catch(const std::exception& e)
    {
        LOG(ERROR) << "exception from MasterServerConnector syncCall getDocumentsByIds(): " << e.what();
        return false;
    }

    return true;
}

} // namespace sf1r

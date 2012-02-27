#include "RemoteItemManager.h"
#include "ItemIdGenerator.h"
#include <query-manager/ActionItem.h>
#include <common/ResultType.h>
#include <aggregator-manager/MasterServerConnector.h>

#include <glog/logging.h>

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
    catch(const msgpack::rpc::rpc_error& e)
    {
        LOG(ERROR) << "failed in MasterServerConnector::Method_getDocumentsByIds_()"
                   << ", exception: " << e.what();
        return false;
    }

    return true;
}

} // namespace sf1r

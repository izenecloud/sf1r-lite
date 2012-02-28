#include "RemoteItemIdGenerator.h"
#include <log-manager/LogServerConnection.h>
#include <log-manager/LogServerRequest.h>

#include <glog/logging.h>
#include <stdexcept>

namespace
{

const std::string& getMethodName(sf1r::LogServerRequest::METHOD method)
{
    return sf1r::LogServerRequest::method_names[method];
}

}

namespace sf1r
{

RemoteItemIdGenerator::RemoteItemIdGenerator(const std::string& collection)
    : connection_(LogServerConnection::instance())
    , collection_(collection)
    , strIdToItemIdMethodName_(getMethodName(LogServerRequest::METHOD_STRID_TO_ITEMID))
    , itemIdToStrIdMethodName_(getMethodName(LogServerRequest::METHOD_ITEMID_TO_STRID))
    , maxItemIdMethodName_(getMethodName(LogServerRequest::METHOD_GET_MAX_ITEMID))
{
}

bool RemoteItemIdGenerator::strIdToItemId(const std::string& strId, itemid_t& itemId)
{
    StrIdToItemIdRequestData request(collection_, strId);

    try
    {
        connection_.syncRequest(strIdToItemIdMethodName_, request, itemId);
    }
    catch(const std::exception& e)
    {
        LOG(ERROR) << "failed in strIdToItemId(), str id: " << strId
                   << ", exception: " << e.what();
        return false;
    }

    return true;
}

bool RemoteItemIdGenerator::itemIdToStrId(itemid_t itemId, std::string& strId)
{
    ItemIdToStrIdRequestData request(collection_, itemId);

    try
    {
        connection_.syncRequest(itemIdToStrIdMethodName_, request, strId);
    }
    catch(const std::exception& e)
    {
        LOG(ERROR) << e.what();
        return false;
    }

    return true;
}

itemid_t RemoteItemIdGenerator::maxItemId() const
{
    itemid_t itemId = 0;

    try
    {
        connection_.syncRequest(maxItemIdMethodName_, collection_, itemId);
    }
    catch(const std::exception& e)
    {
        LOG(ERROR) << "failed in maxItemId(), exception: " << e.what();
    }

    return itemId;
}

} // namespace sf1r

/**
 * @file RemoteItemManager.h
 * @author Jun Jiang
 * @date 2011-04-27
 */

#ifndef REMOTE_ITEM_MANAGER_H
#define REMOTE_ITEM_MANAGER_H

#include "RemoteItemManagerBase.h"

namespace sf1r
{
class MasterServerConnector;

class RemoteItemManager : public RemoteItemManagerBase
{
public:
    RemoteItemManager(
        const std::string& collection,
        ItemIdGenerator* itemIdGenerator
    );

protected:
    virtual bool sendRequest_(
        const GetDocumentsByIdsActionItem& request,
        RawTextResultFromSIA& response
    );

private:
    MasterServerConnector* masterServerConnector_;
};

} // namespace sf1r

#endif // REMOTE_ITEM_MANAGER_H

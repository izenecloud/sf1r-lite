/**
 * @file SearchMasterItemManager.h
 * @author Jun Jiang
 * @date 2011-04-22
 */

#ifndef SEARCH_MASTER_ITEM_MANAGER_H
#define SEARCH_MASTER_ITEM_MANAGER_H

#include "RemoteItemManagerBase.h"

namespace sf1r
{
class IndexSearchService;

class SearchMasterItemManager : public RemoteItemManagerBase
{
public:
    SearchMasterItemManager(
        const std::string& collection,
        ItemIdGenerator* itemIdGenerator,
        IndexSearchService& indexSearchService
    );

protected:
    virtual bool sendRequest_(
        const GetDocumentsByIdsActionItem& request,
        RawTextResultFromSIA& response
    );

private:
    IndexSearchService& indexSearchService_;
};

} // namespace sf1r

#endif // SEARCH_MASTER_ITEM_MANAGER_H

/**
 * @file RemoteItemIdGenerator.h
 * @author Jun Jiang
 * @date 2012-02-14
 */

#ifndef REMOTE_ITEM_ID_GENERATOR_H
#define REMOTE_ITEM_ID_GENERATOR_H

#include "ItemIdGenerator.h"

#include <string>

namespace sf1r
{
class LogServerConnection;

class RemoteItemIdGenerator : public ItemIdGenerator
{
public:
    RemoteItemIdGenerator(const std::string& collection);

    virtual bool strIdToItemId(const std::string& strId, itemid_t& itemId);

    virtual bool itemIdToStrId(itemid_t itemId, std::string& strId);

    virtual itemid_t maxItemId() const;

private:
    LogServerConnection& connection_;

    const std::string collection_;

    const std::string strIdToItemIdMethodName_;
    const std::string itemIdToStrIdMethodName_;
    const std::string maxItemIdMethodName_;
};

} // namespace sf1r

#endif // REMOTE_ITEM_ID_GENERATOR_H

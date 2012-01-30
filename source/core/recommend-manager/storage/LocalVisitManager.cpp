#include "LocalVisitManager.h"

#include <glog/logging.h>

namespace sf1r
{

LocalVisitManager::LocalVisitManager(
    const std::string& visitDBPath,
    const std::string& recommendDBPath,
    const std::string& sessionDBPath
)
    : visitDB_(visitDBPath)
    , recommendDB_(recommendDBPath)
    , sessionDB_(sessionDBPath)
{
    visitDB_.open();
    recommendDB_.open();
    sessionDB_.open();
}

void LocalVisitManager::flush()
{
    try
    {
        visitDB_.flush();
        recommendDB_.flush();
        sessionDB_.flush();
    }
    catch(izenelib::util::IZENELIBException& e)
    {
        LOG(ERROR) << "exception in SDB::flush(): " << e.what();
    }
}

bool LocalVisitManager::getVisitItemSet(const std::string& userId, ItemIdSet& itemIdSet)
{
    return getItemDB_(visitDB_, userId, itemIdSet);
}

bool LocalVisitManager::getRecommendItemSet(const std::string& userId, ItemIdSet& itemIdSet)
{
    return getItemDB_(recommendDB_, userId, itemIdSet);
}

bool LocalVisitManager::getItemDB_(
    ItemDBType& db,
    const std::string& userId,
    ItemIdSet& itemIdSet
)
{
    bool result = false;

    try
    {
        itemIdSet.clear();
        db.getValue(userId, itemIdSet);
        result = true;
    }
    catch(izenelib::util::IZENELIBException& e)
    {
        LOG(ERROR) << "exception in SDB::getValue(): " << e.what();
    }

    return result;
}

bool LocalVisitManager::getVisitSession(const std::string& userId, VisitSession& visitSession)
{
    bool result = false;

    try
    {
        sessionDB_.getValue(userId, visitSession);
        result = true;
    }
    catch(izenelib::util::IZENELIBException& e)
    {
        LOG(ERROR) << "exception in SDB::getValue(): " << e.what();
    }

    return result;
}

bool LocalVisitManager::saveVisitItem_(
    const std::string& userId,
    const ItemIdSet& totalItems,
    itemid_t newItem
)
{
    return saveItemDB_(visitDB_, userId, totalItems);
}

bool LocalVisitManager::saveRecommendItem_(
    const std::string& userId,
    const ItemIdSet& totalItems,
    itemid_t newItem
)
{
    return saveItemDB_(recommendDB_, userId, totalItems);
}

bool LocalVisitManager::saveItemDB_(
    ItemDBType& db,
    const std::string& userId,
    const ItemIdSet& itemIdSet
)
{
    bool result = false;

    try
    {
        result = db.update(userId, itemIdSet);
    }
    catch(izenelib::util::IZENELIBException& e)
    {
        LOG(ERROR) << "exception in SDB::update(): " << e.what();
    }

    return result;
}

bool LocalVisitManager::saveVisitSession_(
    const std::string& userId,
    const VisitSession& visitSession,
    bool isNewSession,
    itemid_t newItem
)
{
    bool result = false;

    try
    {
        result = sessionDB_.update(userId, visitSession);
    }
    catch(izenelib::util::IZENELIBException& e)
    {
        LOG(ERROR) << "exception in SDB::update(): " << e.what();
    }

    return result;
}

} // namespace sf1r

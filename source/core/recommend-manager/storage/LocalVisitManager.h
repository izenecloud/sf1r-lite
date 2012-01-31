/**
 * @file LocalVisitManager.h
 * @author Jun Jiang
 * @date 2011-04-19
 */

#ifndef LOCAL_VISIT_MANAGER_H
#define LOCAL_VISIT_MANAGER_H

#include "VisitManager.h"
#include <sdb/SequentialDB.h>

#include <string>

#include <boost/serialization/set.hpp> // serialize ItemIdSet
#include <boost/serialization/access.hpp>

namespace sf1r
{

class LocalVisitManager : public VisitManager
{
public:
    LocalVisitManager(
        const std::string& visitDBPath,
        const std::string& recommendDBPath,
        const std::string& sessionDBPath
    );

    virtual void flush();

    virtual bool visitRecommendItem(const std::string& userId, itemid_t itemId);

    virtual bool getVisitItemSet(const std::string& userId, ItemIdSet& itemIdSet);

    virtual bool getRecommendItemSet(const std::string& userId, ItemIdSet& itemIdSet);

    virtual bool getVisitSession(const std::string& userId, VisitSession& visitSession);

protected:
    virtual bool saveVisitItem_(const std::string& userId, itemid_t itemId);

    virtual bool saveVisitSession_(
        const std::string& userId,
        const VisitSession& visitSession,
        bool isNewSession,
        itemid_t newItem
    );

private:
    typedef izenelib::sdb::unordered_sdb_tc<std::string, ItemIdSet, ReadWriteLock> ItemDBType;

    bool getItemDB_(
        ItemDBType& db,
        const std::string& userId,
        ItemIdSet& itemIdSet
    );

    bool saveItemDB_(
        ItemDBType& db,
        const std::string& userId,
        const ItemIdSet& itemIdSet
    );

private:
    ItemDBType visitDB_; // the items visited
    ItemDBType recommendDB_; // the items recommended

    typedef izenelib::sdb::unordered_sdb_tc<std::string, VisitSession, ReadWriteLock> SessionDBType;
    SessionDBType sessionDB_; // the items in current session
};

} // namespace sf1r

namespace boost {
namespace serialization {

template<class Archive>
void serialize(Archive& ar, sf1r::VisitSession& visitSession, const unsigned int version)
{
    ar & visitSession.sessionId_;
    ar & visitSession.itemSet_;
}

} // namespace serialization
} // namespace boost

#endif // LOCAL_VISIT_MANAGER_H

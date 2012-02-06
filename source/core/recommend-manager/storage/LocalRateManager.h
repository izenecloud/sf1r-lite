/**
 * @file LocalRateManager.h
 * @author Jun Jiang
 * @date 2012-02-03
 */

#ifndef LOCAL_RATE_MANAGER_H
#define LOCAL_RATE_MANAGER_H

#include "RateManager.h"

#include <sdb/SequentialDB.h>

#include <string>
#include <map>
#include <boost/serialization/map.hpp> // serialize std::map

namespace sf1r
{

class LocalRateManager : public RateManager
{
public:
    LocalRateManager(const std::string& path);

    virtual void flush();

    virtual bool addRate(
        const std::string& userId,
        itemid_t itemId,
        rate_t rate
    );

    virtual bool removeRate(
        const std::string& userId,
        itemid_t itemId
    );

    virtual bool getItemRateMap(
        const std::string& userId,
        ItemRateMap& itemRateMap
    );

private:
    bool saveItemRateMap_(
        const std::string& userId,
        const ItemRateMap& itemRateMap
    );

private:
    typedef izenelib::sdb::unordered_sdb_tc<std::string, ItemRateMap,
                                            ReadWriteLock> SDBType;
    SDBType container_;
};

} // namespace sf1r

#endif // LOCAL_RATE_MANAGER_H

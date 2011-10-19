/**
 * @file RateManager.h
 * @author Jun Jiang
 * @date 2011-10-17
 */

#ifndef RATE_MANAGER_H
#define RATE_MANAGER_H

#include "RecTypes.h"
#include <sdb/SequentialDB.h>

#include <string>
#include <map>

#include <boost/serialization/map.hpp> // serialize std::map

namespace sf1r
{

class RateManager
{
public:
    RateManager(const std::string& path);

    ~RateManager();

    void flush();

    bool addRate(
        userid_t userId,
        itemid_t itemId,
        rate_t rate
    );

    bool removeRate(
        userid_t userId,
        itemid_t itemId
    );

    bool getItemRateMap(
        userid_t userId,
        ItemRateMap& itemRateMap
    );

private:
    typedef izenelib::sdb::unordered_sdb_tc<userid_t, ItemRateMap, ReadWriteLock> SDBType;
    SDBType container_;
};

} // namespace sf1r

#endif // RATE_MANAGER_H

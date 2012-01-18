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
        const std::string& userId,
        itemid_t itemId,
        rate_t rate
    );

    bool removeRate(
        const std::string& userId,
        itemid_t itemId
    );

    bool getItemRateMap(
        const std::string& userId,
        ItemRateMap& itemRateMap
    );

private:
    typedef izenelib::sdb::unordered_sdb_tc<std::string, ItemRateMap, ReadWriteLock> SDBType;
    SDBType container_;
};

} // namespace sf1r

#endif // RATE_MANAGER_H

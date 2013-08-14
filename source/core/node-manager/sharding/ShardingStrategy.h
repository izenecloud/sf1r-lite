/**
 * @file ShardingStrategy.h
 * @author Zhongxia Li
 * @date Nov 3, 2011
 * @brief Strategies (patterns or algorithms) for generating shard id
 */
#ifndef SHARDING_STRATEGY_H_
#define SHARDING_STRATEGY_H_

#include "ShardingConfig.h"

#include <util/ustring/UString.h>
#include <common/Utilities.h>

#include <boost/uuid/sha1.hpp>

#include <vector>
#include <map>

#include <stdint.h>

namespace sf1r{

inline size_t convertUniqueIDForSharding(const std::string& uid)
{
    size_t tmp = 0;
    if (uid.length() == 32)
        tmp = (size_t)Utilities::uuidToUint128(uid);
    else
    {
        try
        {
            tmp = boost::lexical_cast<size_t>(uid);
        }
        catch(const std::exception& e)
        {
            tmp = (size_t)uid[uid.length() - 1];
        }
    }

    return tmp;
}

class ShardingStrategy
{
public:
    typedef std::map<std::string, std::string> ShardFieldListT;
    typedef std::vector<shardid_t>  ShardIDListT;

    ShardingStrategy()
    {
    }
    virtual bool init() = 0;
    virtual shardid_t sharding_for_write(const ShardFieldListT& shardFieldList) = 0;
    virtual ShardIDListT sharding_for_get(const ShardFieldListT& shardFieldList) = 0;
    virtual void save() = 0;

    virtual ~ShardingStrategy(){}

    ShardingConfig shard_cfg_;
};

class HashShardingStrategy : public ShardingStrategy
{
public:
    virtual bool init() { return true; }
    virtual shardid_t sharding_for_write(const ShardFieldListT& shardFieldList);
    virtual ShardIDListT sharding_for_get(const ShardFieldListT& shardFieldList);
    virtual void save() {}
};

class MapShardingStrategy : public ShardingStrategy
{
public:
    MapShardingStrategy(const std::string& map_file)
        :map_file_path_(map_file)
    {
    }

    virtual bool init();
    virtual shardid_t sharding_for_write(const ShardFieldListT& shardFieldList);
    virtual ShardIDListT sharding_for_get(const ShardFieldListT& shardFieldList);
    virtual void save();

    static void readShardingMapFile(const std::string& fullpath,
        std::vector<shardid_t>& sharding_map);
    static void saveShardingMapToFile(const std::string& fullpath,
        const std::vector<shardid_t>& sharding_map);

    static const size_t MAX_MAP_SIZE = 0xFFFF;
private:
    shardid_t sharding_for_write(const std::string& uid, const ShardingConfig& shardingConfig);
    std::vector<shardid_t>  saved_sharding_map_;
    std::string map_file_path_;
};

}

#endif /* SHARDING_STRATEGY_H_ */

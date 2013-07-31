/**
 * @file /sf1r-engine/source/core/node-manager/sharding/ShardingConfig.h
 * @author Zhongxia Li
 * @date Mar 19, 2012
 * @brief  
 */

#ifndef SHARDING_CONFIG_H_
#define SHARDING_CONFIG_H_

#include <set>
#include <vector>
#include <map>

#include <boost/algorithm/string.hpp>

namespace sf1r
{
typedef uint32_t shardid_t; // xxx, shard id start with 1 (1, ..., n)

class ShardingConfig
{
public:
    typedef std::vector<int>  RangeListT;
    typedef std::map<std::string, RangeListT> RangeShardKeyContainerT;
    typedef std::vector<std::string> AttrListT;
    typedef std::map<std::string, AttrListT> AttributeShardKeyContainerT;
    ShardingConfig()
    {}

    // note : the range property used for sharding should be unchangable.
    void addRangeShardKey(const std::string& prop_key, const RangeListT& range_list)
    {
    }
    // note : the attribute property used for sharding should be unchangable.
    void addAttributeShardKey(const std::string& prop_key, const AttrListT& attr_list)
    {
    }
    void setUniqueShardKey(const std::string& prop_key)
    {
        unique_shardkey_ = prop_key;
    }
    bool isRangeShardKey(const std::string& prop_key) const
    {
        return range_shardkeys_.find(prop_key) != range_shardkeys_.end();
    }
    bool isAttributeShardKey(const std::string& prop_key) const
    {
        return attribute_shardkeys_.find(prop_key) != attribute_shardkeys_.end();
    }
    bool isUniqueShardKey(const std::string& prop_key) const
    {
        return unique_shardkey_ == prop_key;
    }

public:

    std::vector<shardid_t> shardidList_;

    std::string unique_shardkey_;
    RangeShardKeyContainerT range_shardkeys_;
    AttributeShardKeyContainerT attribute_shardkeys_;
};

}

#endif /* SHARDING_CONFIG_H_ */

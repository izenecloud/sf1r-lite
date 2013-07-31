#include "ShardingStrategy.h"

#include <common/Utilities.h>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include <glog/logging.h>

#include <3rdparty/udt/md5.h>
//#include <openssl/sha.h>

using namespace sf1r;

shardid_t HashShardingStrategy::sharding_for_write(const ShardFieldListT& shardFieldList, const ShardingConfig& shardingConfig)
{
    std::pair<size_t, size_t> shard_range;
    shard_range.first = 0;
    shard_range.second = shardingConfig.shardidList_.size();
    //for(ShardingConfig::RangeShardKeyContainerT::const_iterator cit = shardingConfig.range_shardkeys_.begin();
    //    cit != shardingConfig.range_shardkeys_.end(); ++cit)
    //{
    //    ShardFieldListT::const_iterator sfit = shardFieldList.find(cit->first);
    //    if (sfit != shardFieldList.end())
    //    {
    //        // chose the range for shardid.
    //        int value = boost::lexical_cast<int>(sfit->second);
    //        if (value < cit->second.front())
    //            shard_range.second = shard_range.first + 1;
    //        else if (value > cit->second.back())
    //            shard_range.first = shard_range.second - 1;
    //        else
    //        {
    //            // determine the value lies in which range and get the range for shard.
    //        }
    //    }
    //    else
    //    {
    //        // empty value write to the first range.
    //        shard_range.second = shard_range.first + 1;
    //    }
    //}
    //for(ShardingConfig::AttributeShardKeyContainerT::const_iterator cit = shardingConfig.attribute_shardkeys_.begin();
    //    cit != shardingConfig.attribute_shardkeys_.end(); ++cit)
    //{
    //    ShardFieldListT::const_iterator sfit = shardFieldList.find(cit->first);
    //    if (sfit != shardFieldList.end())
    //    {
    //        // chose the range for shardid.
    //        if (sfit->second <= cit->second.front())
    //            shard_range.second = shard_range.first + 1;
    //        else if (sfit->second >= cit->second.back())
    //            shard_range.first = shard_range.second - 1;
    //        else
    //        {
    //            // determine the value lies in which range and get the range for shard.
    //        }
    //    }
    //    else
    //    {
    //        // empty value write to the first range.
    //        shard_range.second = shard_range.first + 1;
    //    }
    //}

    // in order to avoid too much migrating while add/remove shard node,
    // we can save the the map from docid to shardid. And save the map to file on HDFS.
    ShardFieldListT::const_iterator sfit = shardFieldList.find(shardingConfig.unique_shardkey_);
    if (sfit == shardFieldList.end())
    {
        LOG(ERROR) << "The shard unique key must exist.";
        return 0;
    }

    shardid_t shardIndex;
    // get shard id from static map first to avoid migrating if add new shard node.
    //std::map<std::string, shardid_t>::const_iterator it = static_map.find(sfit->second.substr(sfit->second.size() - 2));
    //if (it != static_map.end())
    //{
    //    shardIndex = it->second;
    //}
    //else
    //
    shardIndex = shard_range.first + Utilities::uuidToUint128(sfit->second) % (shard_range.second - shard_range.first);

    //
    // static_map[sfit->second.substr(sfit->second.size() - 2)] = shardIndex;
    return shardingConfig.shardidList_[shardIndex];
}

HashShardingStrategy::ShardIDListT HashShardingStrategy::sharding_for_get(const ShardFieldListT& shardFieldList, const ShardingConfig& shardingConfig)
{
    std::pair<size_t, size_t> shard_range;
    shard_range.first = 0;
    shard_range.second = shardingConfig.shardidList_.size();
    ShardIDListT shardids_for_get;
    // check range, if no range given, we need get the shard ranges for each different range.
    //
    //
    // check attribute, if no attribute given, we need get shard ranges for each attribute.
    //
    //
    // check unique id, for each range above get the unique shard in that range.
    // if no unique id, all shards in the ranges should be returned.
    ShardFieldListT::const_iterator sfit = shardFieldList.find(shardingConfig.unique_shardkey_);
    if (sfit != shardFieldList.end())
    {
        shardid_t shardIndex = shard_range.first + Utilities::uuidToUint128(sfit->second) % (shard_range.second - shard_range.first);
        shardids_for_get.push_back(shardingConfig.shardidList_[shardIndex]);
        return shardids_for_get;
    }

    return shardingConfig.shardidList_;
}

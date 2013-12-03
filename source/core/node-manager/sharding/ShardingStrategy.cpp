#include "ShardingStrategy.h"

#include <sstream>
#include <boost/lexical_cast.hpp>
#include <glog/logging.h>

#include <map>
#include <fstream>
#include <3rdparty/udt/md5.h>
#include <boost/filesystem.hpp>

//#define MAX_INDEX  0xFFFF
using namespace sf1r;

shardid_t HashShardingStrategy::sharding_for_write(const ShardFieldListT& shardFieldList)
{
    if (shard_cfg_.shardidList_.empty())
        return 0;
    if (shard_cfg_.shardidList_.size() == 1)
        return shard_cfg_.shardidList_.back();
    std::pair<size_t, size_t> shard_range;
    shard_range.first = 0;
    shard_range.second = shard_cfg_.shardidList_.size();
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
    ShardFieldListT::const_iterator sfit = shardFieldList.find(shard_cfg_.unique_shardkey_);
    if (sfit == shardFieldList.end())
    {
        LOG(ERROR) << "The shard unique key must exist.";
        return 0;
    }

    size_t shardIndex = convertUniqueIDForSharding(sfit->second);
    shardIndex = shard_range.first + shardIndex % (shard_range.second - shard_range.first);

    return shard_cfg_.shardidList_[shardIndex];
}

HashShardingStrategy::ShardIDListT HashShardingStrategy::sharding_for_get(const ShardFieldListT& shardFieldList)
{
    ShardIDListT shardids_for_get;
    if (shard_cfg_.shardidList_.empty())
        return shardids_for_get;
    if (shard_cfg_.shardidList_.size() == 1)
    {
        shardids_for_get.push_back(shard_cfg_.shardidList_.back());
        return shardids_for_get;
    }
    std::pair<size_t, size_t> shard_range;
    shard_range.first = 0;
    shard_range.second = shard_cfg_.shardidList_.size();
    // check range, if no range given, we need get the shard ranges for each different range.
    //
    //
    // check attribute, if no attribute given, we need get shard ranges for each attribute.
    //
    //
    // check unique id, for each range above get the unique shard in that range.
    // if no unique id, all shards in the ranges should be returned.
    ShardFieldListT::const_iterator sfit = shardFieldList.find(shard_cfg_.unique_shardkey_);
    if (sfit != shardFieldList.end())
    {
        size_t shardIndex = convertUniqueIDForSharding(sfit->second);
        shardIndex = shard_range.first + shardIndex % (shard_range.second - shard_range.first);
        shardids_for_get.push_back(shard_cfg_.shardidList_[shardIndex]);
        return shardids_for_get;
    }

    return shard_cfg_.shardidList_;
}

const size_t MapShardingStrategy::MAX_MAP_SIZE;

bool MapShardingStrategy::init()
{
    if (map_file_path_.empty())
        return false;
    if (!boost::filesystem::exists(map_file_path_))
    {
        LOG(INFO) << "no sharding map, prepare it for the first time.";
        // 
        if (shard_cfg_.shardidList_.size() <= 1)
            return true;
        saved_sharding_map_.clear();
        saved_sharding_map_.resize(MAX_MAP_SIZE, 0);
        for(size_t i = 0; i < MAX_MAP_SIZE; ++i)
        {
            saved_sharding_map_[i] = shard_cfg_.shardidList_[i % shard_cfg_.shardidList_.size()];
        }
        save();
    }
    else
    {
        readShardingMapFile(map_file_path_, saved_sharding_map_);
    }
    return true;
}

void MapShardingStrategy::save()
{
    if (saved_sharding_map_.empty())
        return;
    saveShardingMapToFile(map_file_path_, saved_sharding_map_);
}

shardid_t MapShardingStrategy::sharding_for_write(const ShardFieldListT& shardFieldList)
{
    // in order to avoid too much migrating while add/remove shard node,
    // we can save the the map from docid to shardid. And save the map to file on HDFS.
    ShardFieldListT::const_iterator sfit = shardFieldList.find(shard_cfg_.unique_shardkey_);
    if (sfit == shardFieldList.end())
    {
        LOG(ERROR) << "The shard unique key must exist.";
        return 0;
    }

    return sharding_for_write(sfit->second, shard_cfg_);
}

shardid_t MapShardingStrategy::sharding_for_write(const std::string& uid, const ShardingConfig& shardingConfig)
{
    if (shardingConfig.shardidList_.size() == 1)
        return shardingConfig.shardidList_.back();

    if (saved_sharding_map_.empty())
        return 0;

    size_t index = convertUniqueIDForSharding(uid);
    return saved_sharding_map_[index % saved_sharding_map_.size()];
}

MapShardingStrategy::ShardIDListT MapShardingStrategy::sharding_for_get(const ShardFieldListT& shardFieldList)
{
    ShardIDListT shardids_for_get;
    ShardFieldListT::const_iterator sfit = shardFieldList.find(shard_cfg_.unique_shardkey_);
    if (sfit != shardFieldList.end())
    {
        shardid_t selected_id = sharding_for_write(sfit->second, shard_cfg_);
        if (selected_id > 0)
            shardids_for_get.push_back(selected_id);
        return shardids_for_get;
    }

    return shard_cfg_.shardidList_;
}

void MapShardingStrategy::readShardingMapFile(const std::string& fullpath,
    std::vector<shardid_t>& sharding_map)
{
    std::ifstream ifs;
    try
    {
        ifs.open(fullpath.c_str());
        if (!ifs.is_open())
            return;
        shardid_t shardid = 0;
        sharding_map.clear();
        sharding_map.reserve(MAX_MAP_SIZE);
        while (ifs.good())
        {
            uint32_t tmpid = 0;
            ifs >> tmpid;
            shardid = (shardid_t)tmpid;
            if (shardid != 0)
                sharding_map.push_back(shardid);
            if (sharding_map.size() == MAX_MAP_SIZE)
                break;
        }
        if (sharding_map.size() != MAX_MAP_SIZE)
        {
            LOG(WARNING) << "reading sharding map size is wrong." << sharding_map.size();
            sharding_map.clear();
        }
    }
    catch(const std::exception& e)
    {
    }
    if (ifs.is_open())
        ifs.close();
}

void MapShardingStrategy::saveShardingMapToFile(const std::string& fullpath,
    const std::vector<shardid_t>& sharding_map)
{
    if (sharding_map.size() != MAX_MAP_SIZE)
    {
        LOG(WARNING) << "the sharding map size is wrong." << sharding_map.size();
        return;
    }
    if (boost::filesystem::exists(fullpath))
        boost::filesystem::remove(fullpath);
    std::ofstream ofs;
    ofs.open(fullpath.c_str());
    for(size_t i = 0; i < sharding_map.size(); ++i)
    {
        if (sharding_map[i] > 0)
        {
            ofs << (uint32_t)sharding_map[i] << std::endl;
        }
    }
    ofs.close();

}

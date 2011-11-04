#include "ShardingStrategy.h"

#include <sstream>

#include <3rdparty/udt/md5.h>

using namespace sf1r;

shardid_t HashShardingStrategy::sharding(ShardFieldListT& shardFieldList, ShardingParams& shardingParams)
{
    std::cout<<"HashShardingStrategy shardkeys:";

    // combine shard keys
    std::ostringstream oss;
    for (ShardFieldListT::iterator it = shardFieldList.begin(); it != shardFieldList.end(); it++)
    {
        oss << it->second;
    }
    std::string shardkeys = oss.str();
    std::cout<<shardkeys<<std::endl;

    // hash, todo
    md5_state_t st;
    md5_init(&st);
    md5_append(&st, (const md5_byte_t*)shardkeys.c_str(), shardkeys.size());
    md5_byte_t digest[16];
    memset(digest, 0, sizeof(digest));
    md5_finish(&st,digest);

//    for(int i=0;i<16;++i)
//    {
//        std::cout<<hex<<(int)digest[i];
//    }
//    std::cout<<std::endl;

    uint64_t ui = *((uint64_t*)digest);

    // get shard id
    shardid_t shardid = ui % shardingParams.shardNum_;
    std::cout <<" shardid: "<< shardid+1 << std::endl;

    return (shardid+1);
}

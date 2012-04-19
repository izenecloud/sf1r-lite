#include "ShardingStrategy.h"

#include <sstream>

#include <3rdparty/udt/md5.h>
//#include <openssl/sha.h>

using namespace sf1r;

shardid_t HashShardingStrategy::sharding(ShardFieldListT& shardFieldList, ShardingParams& shardingParams)
{
    // combine shard keys
    std::string shardkeys;
    for (ShardFieldListT::iterator it = shardFieldList.begin(); it != shardFieldList.end(); it++)
    {
        shardkeys.append(it->second);
    }

    // string to number
    uint64_t ui = hashmd5(shardkeys.c_str(), shardkeys.size());

    // get shard id
    shardid_t shardid = ui % shardingParams.shardNum_;

    return (shardid+1);
}

uint64_t HashShardingStrategy::hashmd5(const char* data, unsigned long len)
{
    static int MD5_DIGEST_LENGTH = 16; // 16 byte (128 bit)

    md5_state_t st;
    md5_init(&st);
    md5_append(&st, (const md5_byte_t*)data, len);
    md5_byte_t digest[MD5_DIGEST_LENGTH];
    memset(digest, 0, sizeof(digest));
    md5_finish(&st,digest);

    return *((uint64_t*)digest);
}

uint64_t HashShardingStrategy::hashsha(const char* data, unsigned long len)
{
    sha1_.reset();
    sha1_.process_bytes(data, len);
    unsigned int digest[5];
    boost::uuids::detail::sha1::digest_type rdigest = digest;
    sha1_.get_digest(digest);

    return *((uint64_t*)rdigest);
}

#include "ShardingStrategy.h"

#include <sstream>

#include <3rdparty/udt/md5.h>
//#include <openssl/sha.h>

using namespace sf1r;

shardid_t HashShardingStrategy::sharding(ShardFieldListT& shardFieldList, ShardingParams& shardingParams)
{
    ///std::cout<<"HashShardingStrategy shardkeys: ";

    // combine shard keys, xxx
    std::string shardkeys;
    for (ShardFieldListT::iterator it = shardFieldList.begin(); it != shardFieldList.end(); it++)
    {
        shardkeys.append(it->second);
    }
    ///std::cout<<shardkeys<<std::endl;

    // string hash, xxx
    uint64_t ui = hashmd5(shardkeys.c_str(), shardkeys.size());

    // get shard id
    shardid_t shardid = ui % shardingParams.shardNum_;
    ///std::cout <<"--> shardid: "<< shardid+1 << std::endl;

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

//    for(int i=0;i<16;++i)
//    {
//        std::cout<<hex<<(int)digest[i];
//    }
//    std::cout<<std::endl;

    return *((uint64_t*)digest);
}

uint64_t HashShardingStrategy::hashsha(const char* data, unsigned long len)
{
    //unsigned char buf[SHA_DIGEST_LENGTH];
    //SHA1((unsigned const char*)data, len, buf);

    sha1_.reset();
    sha1_.process_bytes(data, len);
    unsigned int digest[5];
    boost::uuids::detail::sha1::digest_type rdigest = digest;
    sha1_.get_digest(digest);

//    for(int i=0;i<20;++i)
//    {
//        std::cout<<hex<<(int)((unsigned char*)digest)[i];
//    }
//    std::cout<<std::endl;

    return *((uint64_t*)rdigest);
}

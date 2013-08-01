#include <stdlib.h>

#include <node-manager/sharding/ScdSharder.h>
#include <node-manager/sharding/ShardingStrategy.h>
#include <node-manager/sharding/ScdDispatcher.h>

using namespace std;
using namespace sf1r;

int main(int argc, char** argv)
{
    bool help = false;
    char optchar;
    std::string dir = "/home/vincentlee/codebase/sf1r-engine/bin/collection/chinese-wiki/scd/index";
    int maxDoc = 0;
    ShardingConfig cfg;
    ShardingConfig::RangeListT ranges;
    ranges.push_back(10);
    ranges.push_back(100);
    ranges.push_back(1000);
    cfg.addRangeShardKey("Price", ranges);
    ShardingConfig::AttrListT strranges;
    strranges.push_back("bbb");
    strranges.push_back("eee");
    cfg.addAttributeShardKey("Attribute", strranges);
    cfg.setUniqueShardKey("DOCID");

    while ((optchar = getopt(argc, argv, "hd:n:s:m:")) != -1)
    {
        switch (optchar) {
            case 'd':
                dir = optarg;
                break;
            case 'm':
                maxDoc = atoi(optarg);
                break;
            case 'h':
                help = true;
                break;
            default:
                cout << "Unrecognized flag " << optchar << endl;
                help = true;
                break;
        }
    }

    if (true)
    {
        cout << "Usage: " << argv[0] << " -d <scd dir> -m <max doc> -n <shard number> -s <shardkey1>:<shardkey2>:<...> "
             << endl;

        if (help)
            return 0;
    }

    cfg.shardidList_.push_back(1);
    cfg.shardidList_.push_back(2);
    cfg.shardidList_.push_back(3);
    // show status
    cout<<"----------------------"<<endl;
    cout<<"scd dir: "<<dir<<endl;
    cout<<"documents(0 for all): "<<maxDoc<<endl;
    //cout<<"shards number: "<<cfg.getShardNum()<<endl;
    cout<<"shard keys: ";
    //
    // create scd sharding
    boost::shared_ptr<ScdSharder> scdSharder(new ScdSharder);
    if (!scdSharder->init(cfg))
    {
        return 0;
    }

    // create scd dispatcher
    boost::shared_ptr<ScdDispatcher> scdDispatcher(new BatchScdDispatcher(scdSharder, "chinese-wiki", false));

    // In this test, there is no Worker and scd transmission will fail,
    // but it dosen't matter, because we only need to test sharding.
    std::vector<std::string> outScdFileList;
    scdDispatcher->dispatch(outScdFileList, dir, dir + "/out", maxDoc);

    return 0;
}

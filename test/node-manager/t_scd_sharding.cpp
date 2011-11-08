#include <stdlib.h>

#include <node-manager/ScdSharding.h>
#include <node-manager/ShardingStrategy.h>
#include <node-manager/ScdDispatcher.h>

using namespace std;
using namespace sf1r;

int main(int argc, char** argv)
{
    bool help = false;
    char optchar;
    std::string dir = "/home/zhongxia/codebase/sf1r-engine/bin/collection/chinese-wiki/scd/index";
    int maxDoc = 0;
    ShardingConfig cfg;
    cfg.setShardNum(3);
    cfg.addShardKey("Url");
    cfg.addShardKey("Title");

    while ((optchar = getopt(argc, argv, "hd:n:s:m:")) != -1)
    {
        switch (optchar) {
            case 'd':
                dir = optarg;
                break;
            case 'n':
                cfg.setShardNum(atoi(optarg));
                break;
            case 'm':
                maxDoc = atoi(optarg);
                break;
            case 's':
                {
                    // reset
                    cfg.clearShardKeys();
                    char* p = optarg;
                    std::string shardkey;
                    while (*p)
                    {
                        if (*p == ':')
                        {
                            if (!shardkey.empty())
                            {
                                cfg.addShardKey(shardkey);
                                //cout<<"shardkey: "<<shardkey<<endl;
                            }
                            shardkey.clear();
                            p++;
                            continue;
                        }

                        shardkey.push_back(*p++);
                    }

                    if (!shardkey.empty())
                    {
                        cfg.addShardKey(shardkey);
                        //cout<<"shardkey: "<<shardkey<<endl;
                    }
                }
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

    // show status
    cout<<"----------------------"<<endl;
    cout<<"scd dir: "<<dir<<endl;
    cout<<"documents(0 for all): "<<maxDoc<<endl;
    cout<<"shards number: "<<cfg.getShardNum()<<endl;
    cout<<"shard keys: ";
    std::set<std::string>&keys = cfg.getShardKeys();
    std::set<std::string>::iterator it;
    for (it = keys.begin(); it != keys.end(); it++)
    {
        cout<<*it<<" ";
    }
    cout<<endl<<"----------------------"<<endl<<endl;

    // create scd sharding
    ShardingStrategy* shardingStrategy = new HashShardingStrategy;
    ScdSharding scdSharding(cfg, shardingStrategy);

    // create scd dispatcher
    ScdDispatcher* scdDispatcher = new BatchScdDispatcher(&scdSharding);

    scdDispatcher->dispatch(dir, maxDoc);

    delete shardingStrategy;
    delete scdDispatcher;

    return 0;
}

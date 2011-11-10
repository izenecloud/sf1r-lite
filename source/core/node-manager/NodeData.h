/**
 * @file NodeData.h
 * @author Zhongxia Li
 * @date Nov 9, 2011
 * @brief 
 */
#ifndef NODE_DATA_H_
#define NODEDATA_H_

#include <3rdparty/zookeeper/ZkDataPack.hpp>

namespace sf1r {

typedef zookeeper::ZkDataPack NodeData;

const static char* NDATA_KEY_HOST = "host";
const static char* NDATA_KEY_BA_PORT = "baport";
const static char* NDATA_KEY_MASTER_PORT = "mport";
const static char* NDATA_KEY_WORKER_PORT = "wport";
const static char* NDATA_KEY_SHARD_ID = "shardid";

}

#endif /* NODE_DATA_H_ */

#include "ZooKeeperNamespace.h"


namespace sf1r {

/// ZooKeeper Namespace
std::string ZooKeeperNamespace::sf1rCluster_ = "/SF1R-unknown";  // specify by configuration

const std::string ZooKeeperNamespace::primary_ = "/Primary";
const std::string ZooKeeperNamespace::searchPrimaryNodes_ = "/SearchPrimaryNodes";
const std::string ZooKeeperNamespace::searchTopology_ = "/SearchTopology";
const std::string ZooKeeperNamespace::searchServers_ = "/SearchServers";
const std::string ZooKeeperNamespace::recommendPrimaryNodes_ = "/RecommendPrimaryNodes";
const std::string ZooKeeperNamespace::recommendTopology_ = "/RecommendTopology";
const std::string ZooKeeperNamespace::recommendServers_ = "/RecommendServers";
const std::string ZooKeeperNamespace::replica_ = "/Replica";
const std::string ZooKeeperNamespace::node_ = "/Node";
const std::string ZooKeeperNamespace::server_ = "/Server";

const std::string ZooKeeperNamespace::Synchro_ = "/Synchro";

/// ZooKeeper Node data key
const char* ZNode::KEY_USERNAME = "username";
const char* ZNode::KEY_HOST = "host";
const char* ZNode::KEY_BA_PORT = "baport";
const char* ZNode::KEY_DATA_PORT = "dataport";
const char* ZNode::KEY_MASTER_NAME = "mastername";
const char* ZNode::KEY_MASTER_PORT = "masterport";
const char* ZNode::KEY_WORKER_PORT = "workerport";
const char* ZNode::KEY_SHARD_ID = "shardid";
const char* ZNode::KEY_REPLICA_ID = "replicaid";
const char* ZNode::KEY_COLLECTION = "collection";
const char* ZNode::KEY_NODE_STATE = "nodestate";
const char* ZNode::KEY_SELF_REG_PRIMARY_PATH = "self_primary_nodepath";

const char* ZNode::KEY_FILE = "file";
const char* ZNode::KEY_DIR = "dir";


}

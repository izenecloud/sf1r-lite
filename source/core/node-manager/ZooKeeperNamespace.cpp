#include "ZooKeeperNamespace.h"


namespace sf1r {

/// ZooKeeperNamespace
std::string ZooKeeperNamespace::sf1rCluster_ = "/SF1R-unknown";    // identify SF1R cluster, configured
const std::string ZooKeeperNamespace::sf1rTopology_ = "/Topology";
const std::string ZooKeeperNamespace::sf1rService_ = "/Service";

/// ZooKeeper Node data key
const char* ZNode::KEY_HOST = "host";
const char* ZNode::KEY_BA_PORT = "baport";
const char* ZNode::KEY_DATA_PORT = "dataport";
const char* ZNode::KEY_NOTIFY_PORT = "notifyport";
const char* ZNode::KEY_WORKER_PORT = "workerport";
const char* ZNode::KEY_SHARD_ID = "shardid";

const char* ZNode::KEY_FILE = "file";
const char* ZNode::KEY_DIR = "dir";


}

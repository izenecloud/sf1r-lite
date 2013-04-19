#include "ZooKeeperNamespace.h"


namespace sf1r {

/// ZooKeeper Namespace
std::string ZooKeeperNamespace::sf1rCluster_ = "/SF1R-unknown";  // specify by configuration

const std::string ZooKeeperNamespace::primary_ = "/Primary";
const std::string ZooKeeperNamespace::primaryNodes_ = "/PrimaryNodes";
const std::string ZooKeeperNamespace::write_req_queue_ = "/WriteRequestQueue";
const std::string ZooKeeperNamespace::topology_ = "/Topology";
const std::string ZooKeeperNamespace::servers_ = "/Servers";
const std::string ZooKeeperNamespace::replica_ = "/Replica";
const std::string ZooKeeperNamespace::node_ = "/Node";
const std::string ZooKeeperNamespace::server_ = "/Server";

const std::string ZooKeeperNamespace::Synchro_ = "/Synchro";
const std::string ZooKeeperNamespace::write_req_prepare_node_ = "/WriteRequestPrepare";
const std::string ZooKeeperNamespace::write_req_seq_ = "/WriteRequestSeq";

/// ZooKeeper Node data key
const char* ZNode::KEY_USERNAME = "username";
const char* ZNode::KEY_HOST = "host";
const char* ZNode::KEY_BA_PORT = "baport";
const char* ZNode::KEY_DATA_PORT = "dataport";
const char* ZNode::KEY_MASTER_NAME = "mastername";
const char* ZNode::KEY_MASTER_PORT = "masterport";
const char* ZNode::KEY_WORKER_PORT = "workerport";
const char* ZNode::KEY_FILESYNC_RPCPORT = "filesync_rpcport";
const char* ZNode::KEY_REPLICA_ID = "replicaid";
const char* ZNode::KEY_COLLECTION = "collection";
const char* ZNode::KEY_NODE_STATE = "nodestate";
const char* ZNode::KEY_SELF_REG_PRIMARY_PATH = "self_primary_nodepath";
const char* ZNode::KEY_MASTER_SERVER_REAL_PATH = "master_server_realpath";
const char* ZNode::KEY_PRIMARY_WORKER_REQ_DATA = "primary_worker_req_data";
const char* ZNode::KEY_REQ_DATA = "req_data";
const char* ZNode::KEY_REQ_TYPE = "req_type";
const char* ZNode::KEY_LAST_WRITE_REQID = "req_last_id";
const char* ZNode::KEY_REQ_STEP = "req_step";
const char* ZNode::KEY_SERVICE_STATE = "service_state";
const char* ZNode::KEY_SERVICE_NAMES = "service_names";

const char* ZNode::KEY_FILE = "file";
const char* ZNode::KEY_DIR = "dir";


}

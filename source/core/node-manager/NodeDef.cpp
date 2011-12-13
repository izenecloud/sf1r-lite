#include "NodeDef.h"


namespace sf1r {

std::string NodeDef::sf1rCluster_ = "/SF1R-unknown";         // identify SF1R cluster, configured
const std::string NodeDef::sf1rTopology_ = "/Topology";
const std::string NodeDef::sf1rService_ = "/Service";

/// NodeData
const char* NodeData::NDATA_KEY_HOST = "host";
const char* NodeData::NDATA_KEY_BA_PORT = "baport";
const char* NodeData::NDATA_KEY_DATA_PORT = "dataport";
const char* NodeData::NDATA_KEY_NOTIFY_PORT = "notifyport";
const char* NodeData::NDATA_KEY_WORKER_PORT = "wport";
const char* NodeData::NDATA_KEY_SHARD_ID = "shardid";

const char* NodeData::ND_KEY_SYNC_PRODUCER = "sync-p";
const char* NodeData::ND_KEY_SYNC_CONSUMER = "sync-c";

const char* NodeData::ND_KEY_FILE = "file";
const char* NodeData::ND_KEY_DIR = "dir";
}

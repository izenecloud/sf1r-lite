#include "SynchroData.h"

namespace sf1r{

/// SynchroData
const char* SynchroData::KEY_HOST = "host";
const char* SynchroData::KEY_DATA_PORT = "port";
const char* SynchroData::KEY_DATA_TYPE = "datatype";
const char* SynchroData::DATA_TYPE_SCD_INDEX = "scdindex";
const char* SynchroData::COMMENT_TYPE_FLAG = "commentflag";
const char* SynchroData::KEY_DATA_PATH = "path";
const char* SynchroData::KEY_COLLECTION = "colletion";
const char* SynchroData::KEY_ERROR = "error";
const char* SynchroData::KEY_RETURN = "return";

const char* SynchroData::KEY_CONSUMER_STATUS = "consumer_status";
const char* SynchroData::CONSUMER_STATUS_READY = "ready";
const char* SynchroData::CONSUMER_STATUS_RECEIVE_SUCCESS = "receive_success";
const char* SynchroData::CONSUMER_STATUS_RECEIVE_FAILURE = "receive_failure";

/// SynchroZkNode
const char* SynchroZkNode::PRODUCER = "/Producer";
const char* SynchroZkNode::CONSUMER = "/Consumer";

}


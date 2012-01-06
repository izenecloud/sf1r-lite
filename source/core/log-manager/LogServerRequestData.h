#ifndef LOG_SERVER_REQUEST_DATA_H_
#define LOG_SERVER_REQUEST_DATA_H_

#include <3rdparty/msgpack/msgpack.hpp>

#include <string>
#include <vector>

namespace sf1r
{

struct LogServerRequestData
{
};

/// xxx modify on practical use
struct UUID2DocidList : public LogServerRequestData
{
    typedef std::vector<uint32_t> DocidListType;

    uint128_t uuid_;
    DocidListType docidList_;

    std::string toString() const;

    MSGPACK_DEFINE(uuid_, docidList_)
};

struct SynchronizeData : public LogServerRequestData
{
    // reserved
    std::string msg_;

    MSGPACK_DEFINE(msg_)
};

}

#endif /* LOG_SERVER_REQUEST_DATA_H_ */

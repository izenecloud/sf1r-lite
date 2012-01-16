#ifndef LOG_SERVER_REQUEST_DATA_H_
#define LOG_SERVER_REQUEST_DATA_H_

#include <3rdparty/msgpack/msgpack.hpp>

#include <string>
#include <vector>

namespace sf1r
{

inline std::ostream& operator<<(std::ostream& os, uint128_t uint128)
{
    os << hex << uint64_t(uint128>>64) << uint64_t(uint128) << dec;
    return os;
}

struct LogServerRequestData
{
};

struct UUID2DocidList : public LogServerRequestData
{
    typedef std::vector<uint128_t> DocidListType;

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

struct Docid2UUID : public LogServerRequestData
{
    uint128_t docid_;
    uint128_t uuid_;

    std::string toString() const;

    MSGPACK_DEFINE(docid_, uuid_)
};

}

#endif /* LOG_SERVER_REQUEST_DATA_H_ */

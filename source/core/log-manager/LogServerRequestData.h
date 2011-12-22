#ifndef LOG_SERVER_REQUEST_DATA_H_
#define LOG_SERVER_REQUEST_DATA_H_

#include <string>
#include <vector>

#include <3rdparty/msgpack/msgpack.hpp>

namespace sf1r
{

struct LogServerRequestData
{
};

/// xxx modify on practical usage
struct UUID2DocIdList : public LogServerRequestData
{
    std::string uuid_;
    std::vector<uint32_t> docIdList_;

    MSGPACK_DEFINE(uuid_, docIdList_)
};

}

#endif /* LOG_SERVER_REQUEST_DATA_H_ */

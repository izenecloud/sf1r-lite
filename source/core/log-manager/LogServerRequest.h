#ifndef _LOG_SERVER_REQUEST_H_
#define _LOG_SERVER_REQUEST_H_

#include <string>
#include <vector>

#include <3rdparty/msgpack/msgpack.hpp>

namespace sf1r
{

class LogServerRequest
{
public:
    LogServerRequest() {}

public:
    typedef std::string method_t;

    static const char* METHOD_UPDATE_UUID;
};

class UUID2DocIdList : public LogServerRequest
{
public:
    std::string uuid_; // xxx
    std::vector<uint32_t> docIdList_;

    MSGPACK_DEFINE(uuid_, docIdList_)
};

}

#endif /* _LOG_SERVER_REQUEST_H_ */

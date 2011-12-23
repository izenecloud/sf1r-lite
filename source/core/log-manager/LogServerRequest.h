#ifndef _LOG_SERVER_REQUEST_H_
#define _LOG_SERVER_REQUEST_H_

#include "LogServerRequestData.h"

namespace sf1r
{

class LogServerRequest
{
public:
    typedef std::string method_t;

    /// add method here
    static const char* METHOD_UPDATE_UUID;

    method_t method_;

public:
    LogServerRequest(const method_t& method) : method_(method) {}
    virtual ~LogServerRequest() {}
};

template <typename RequestDataT>
class LogRequestRequestT : public LogServerRequest
{
public:
    LogRequestRequestT(method_t method)
        :LogServerRequest(method)
    {
    }

    RequestDataT param_;
};

class UpdateUUIDRequest : public LogRequestRequestT<UUID2DocIdList>
{
public:
    UpdateUUIDRequest()
        : LogRequestRequestT<UUID2DocIdList>(METHOD_UPDATE_UUID)
    {
    }
};

}

#endif /* _LOG_SERVER_REQUEST_H_ */

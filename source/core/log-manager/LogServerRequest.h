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
    enum METHOD
    {
        METHOD_UPDATE_UUID = 0,
        METHOD_SYNCHRONIZE,
        COUNT_OF_METHODS
    };

    static const method_t method_names[COUNT_OF_METHODS];

    METHOD method_;

public:
    LogServerRequest(const METHOD& method) : method_(method) {}
    virtual ~LogServerRequest() {}
};

template <typename RequestDataT>
class LogRequestRequestT : public LogServerRequest
{
public:
    LogRequestRequestT(METHOD method)
        :LogServerRequest(method)
    {
    }

    RequestDataT param_;
};

class UpdateUUIDRequest : public LogRequestRequestT<UUID2DocidList>
{
public:
    UpdateUUIDRequest()
        : LogRequestRequestT<UUID2DocidList>(METHOD_UPDATE_UUID)
    {
    }
};

class SynchronizeRequest : public LogRequestRequestT<SynchronizeData>
{
public:
    SynchronizeRequest()
        : LogRequestRequestT<SynchronizeData>(METHOD_SYNCHRONIZE)
    {
    }
};

}

#endif /* _LOG_SERVER_REQUEST_H_ */

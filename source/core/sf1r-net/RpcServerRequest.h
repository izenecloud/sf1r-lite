#ifndef _RPC_SERVER_REQUEST_H_
#define _RPC_SERVER_REQUEST_H_

#include "RpcServerRequestData.h"
#include <string>

namespace sf1r
{

class RpcServerRequest
{
public:
    int method_;

public:
    RpcServerRequest(const int& method) : method_(method) {}
    virtual ~RpcServerRequest() {}
};

template <typename RequestDataT, typename BaseRequest>
class RpcRequestRequestT : public BaseRequest
{
public:
    RpcRequestRequestT(int method)
        : BaseRequest(method)
    {
    }

    RequestDataT param_;

    MSGPACK_DEFINE(param_)
};

}

#endif /* _RPC_SERVER_REQUEST_H_ */

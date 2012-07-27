#ifndef RPC_SERVER_REQUEST_DATA_H_
#define RPC_SERVER_REQUEST_DATA_H_

#include <3rdparty/msgpack/msgpack.hpp>
namespace sf1r
{

struct RpcServerRequestData
{
};

struct TestData: public RpcServerRequestData
{
    bool ret;
    MSGPACK_DEFINE( ret )
};
}

#endif /* RPC_SERVER_REQUEST_DATA_H_ */

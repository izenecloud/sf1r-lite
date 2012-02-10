///
/// @file RpcConnectionTestFixture.h
/// @brief a fixture used to test LogServerConnection
/// @author Jun Jiang
/// @date Created 2012-02-08
///

#ifndef RPC_CONNECTION_TEST_FIXTURE_H
#define RPC_CONNECTION_TEST_FIXTURE_H

#include "RpcTestServer.h"
#include <log-manager/LogServerConnection.h>

#include <string>

namespace sf1r
{

class RpcConnectionTestFixture
{
public:
    RpcConnectionTestFixture();

    enum MethodType
    {
        REMOTE_ERROR = 0,
        ECHO_STR,
        ADD_INT,
        INCREASE_COUNT,
        METHOD_NUM
    };
    static std::string METHOD_NAMES[METHOD_NUM];

    std::string echo_str(const std::string& param) const;

    struct AddIntParam;
    int add_int(const AddIntParam& param) const;

    void runAddInt(int runTimes);

protected:
    void reg_func_remote_error(msgpack::rpc::request req);
    void reg_func_echo_str(msgpack::rpc::request req);
    void reg_func_add_int(msgpack::rpc::request req);
    void reg_func_increase_count(msgpack::rpc::request req);

protected:
    typedef void (RpcConnectionTestFixture::*RegisterFunc)(msgpack::rpc::request);
    static RegisterFunc REGISTER_FUNCS[METHOD_NUM];

    const std::string host_;
    const int port_;
    const int threadNum_;

    RpcTestServer server_;
    LogServerConnection& connection_;

    int increaseCount_; // used in "reg_func_increase_count()"
};

struct RpcConnectionTestFixture::AddIntParam
{
    int p1;
    int p2;

    AddIntParam() : p1(0), p2(0) {}
    AddIntParam(int i1, int i2) : p1(i1), p2(i2) {}

    MSGPACK_DEFINE(p1, p2)
};

} // namespace sf1r

#endif //RPC_CONNECTION_TEST_FIXTURE_H

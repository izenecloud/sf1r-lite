#include "RpcConnectionTestFixture.h"
#include <util/test/BoostTestThreadSafety.h>

#include <boost/bind.hpp>
#include <cstdlib> // rand()

namespace sf1r
{

std::string RpcConnectionTestFixture::METHOD_NAMES[] =
{
    "echo_str",
    "remote_error",
    "add_int",
    "increase_count"
};

RpcConnectionTestFixture::RegisterFunc RpcConnectionTestFixture::REGISTER_FUNCS[] =
{
    &RpcConnectionTestFixture::reg_func_remote_error,
    &RpcConnectionTestFixture::reg_func_echo_str,
    &RpcConnectionTestFixture::reg_func_add_int,
    &RpcConnectionTestFixture::reg_func_increase_count
};

RpcConnectionTestFixture::RpcConnectionTestFixture()
    : config_("localhost", 19000)
    , connection_(LogServerConnection::instance())
    , increaseCount_(0)
{
    server_.instance.listen(config_.host, config_.rpcPort);
    server_.instance.start(4);

    connection_.init(config_);

    for (int i=0; i<METHOD_NUM; ++i)
    {
        server_.registerMethod(METHOD_NAMES[i],
                               boost::bind(REGISTER_FUNCS[i], this, _1));
    }
}

std::string RpcConnectionTestFixture::echo_str(const std::string& param) const
{
    return param;
}

int RpcConnectionTestFixture::add_int(const AddIntParam& param) const
{
    return param.p1 + param.p2;
}

void RpcConnectionTestFixture::runAddInt(int runTimes)
{
    std::string method(METHOD_NAMES[ADD_INT]);

    for (int i=0; i<runTimes; ++i)
    {
        // generate rand int
        AddIntParam addParam(rand(), rand());
        int addResult = 0;

        connection_.syncRequest(method, addParam, addResult);

        BOOST_CHECK_EQUAL_TS(addResult, add_int(addParam));
    }
}

void RpcConnectionTestFixture::runIncreaseCount(int runTimes)
{
    std::string method(METHOD_NAMES[INCREASE_COUNT]);
    std::string param("void");

    for (int i=0; i<runTimes; ++i)
    {
        connection_.asynRequest(method, param);
    }

    // flush asynchronous calls
    connection_.flushRequests();
}

void RpcConnectionTestFixture::reg_func_echo_str(msgpack::rpc::request req)
{
    msgpack::type::tuple<std::string> params;
    req.params().convert(&params);
    const std::string& msg = params.get<0>();

    req.result(echo_str(msg));
}

void RpcConnectionTestFixture::reg_func_remote_error(msgpack::rpc::request req)
{
    std::string msg("always fail");
    req.error(msg);
}

void RpcConnectionTestFixture::reg_func_add_int(msgpack::rpc::request req)
{
    msgpack::type::tuple<AddIntParam> params;
    req.params().convert(&params);
    const AddIntParam& addParam = params.get<0>();

    req.result(add_int(addParam));
}

void RpcConnectionTestFixture::reg_func_increase_count(msgpack::rpc::request req)
{
    ++increaseCount_;
}

} // namespace sf1r

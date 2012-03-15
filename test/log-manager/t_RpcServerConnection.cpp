///
/// @file t_RpcServerConnection.cpp
/// @brief test LogServerConnection functions
/// @author Jun Jiang
/// @date Created 2012-02-08
///

#include "RpcConnectionTestFixture.h"

#include <boost/test/unit_test.hpp>
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include <unistd.h> // sleep

using namespace std;
using namespace sf1r;

namespace
{

template <typename ExceptionT, typename ParamT, typename ResultT>
void checkRequestException(
    LogServerConnection& connection,
    const std::string& method,
    const ParamT& param,
    ResultT& result
)
{
    BOOST_CHECK_THROW(connection.syncRequest(method, param, result),
                      ExceptionT);

    // no exception is thrown for async request
    connection.asynRequest(method, param);
    // flush asynchronous call
    connection.flushRequests();
}

}

BOOST_FIXTURE_TEST_SUITE(LogServerConnectionTest, RpcConnectionTestFixture)

BOOST_AUTO_TEST_CASE(expectConnectError)
{
    LogServerConnectionConfig newConfig = config_;
    ++newConfig.rpcPort;
    connection_.init(newConfig);

    std::string method(METHOD_NAMES[ECHO_STR]);
    std::string param("hello");
    std::string result;

    checkRequestException<msgpack::rpc::connect_error>(connection_, method, param, result);
}

BOOST_AUTO_TEST_CASE(expectHostResolveError)
{
    LogServerConnectionConfig newConfig;
    newConfig.host.clear();
    connection_.init(newConfig);

    std::string method(METHOD_NAMES[ECHO_STR]);
    std::string param("hello");
    std::string result;

    typedef std::runtime_error ExceptionType;
    BOOST_CHECK_THROW(connection_.syncRequest(method, param, result), ExceptionType);
    BOOST_CHECK_THROW(connection_.asynRequest(method, param), ExceptionType);
}

BOOST_AUTO_TEST_CASE(expectTimeOutError)
{
    std::string method(METHOD_NAMES[ECHO_STR]);
    std::string param("hello");
    std::string result;

    connection_.syncRequest(method, param, result);
    BOOST_CHECK_EQUAL(result, param);

    server_.stop();
    checkRequestException<msgpack::rpc::timeout_error>(connection_, method, param, result);
}

BOOST_AUTO_TEST_CASE(expectNoMethodError)
{
    std::string method("undefined");
    std::string param("void");
    std::string result;

    checkRequestException<msgpack::rpc::no_method_error>(connection_, method, param, result);
}

BOOST_AUTO_TEST_CASE(expectArgumentError)
{
    std::string method(METHOD_NAMES[ECHO_STR]);
    int param = 100;
    std::string result;

    checkRequestException<msgpack::rpc::argument_error>(connection_, method, param, result);
}

BOOST_AUTO_TEST_CASE(expectRemoteError)
{
    std::string method(METHOD_NAMES[REMOTE_ERROR]);
    std::string param("void");
    std::string result;

    checkRequestException<msgpack::rpc::remote_error>(connection_, method, param, result);
}

BOOST_AUTO_TEST_CASE(checkSyncRequest)
{
    std::string param("hello");
    std::string result;

    connection_.syncRequest(METHOD_NAMES[ECHO_STR], param, result);
    BOOST_CHECK_EQUAL(result, echo_str(param));

    AddIntParam addParam(1, 2);
    int addResult = 0;

    connection_.syncRequest(METHOD_NAMES[ADD_INT], addParam, addResult);
    BOOST_CHECK_EQUAL(addResult, add_int(addParam));
}

BOOST_AUTO_TEST_CASE(checkAsynRequest)
{
    int runTimes = 1500;
    runIncreaseCount(runTimes);

    // waiting for a while before checking the effect by asynchronous calls
    sleep(1);
    BOOST_CHECK_EQUAL(increaseCount_, runTimes);
}

BOOST_AUTO_TEST_CASE(checkMultiThreadSyncRequest)
{
    int threadNum = 5;
    int runTimes = 1500;
    boost::thread_group threads;

    for (int i=0; i<threadNum; ++i)
    {
        threads.create_thread(boost::bind(&RpcConnectionTestFixture::runAddInt,
                                          this, runTimes));
    }

    threads.join_all();
}

BOOST_AUTO_TEST_CASE(checkMultiThreadAsynRequest)
{
    int threadNum = 5;
    int runTimes = 1500;
    boost::thread_group threads;

    for (int i=0; i<threadNum; ++i)
    {
        threads.create_thread(boost::bind(&RpcConnectionTestFixture::runIncreaseCount,
                                          this, runTimes));
    }

    threads.join_all();

    // waiting for a while before checking the effect by asynchronous calls
    sleep(1);
    int totalCount = threadNum * runTimes;
    BOOST_CHECK_EQUAL(increaseCount_, totalCount);
}

BOOST_AUTO_TEST_SUITE_END() 

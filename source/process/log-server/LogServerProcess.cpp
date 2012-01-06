#include "LogServerProcess.h"
#include "LogServerStorage.h"

#include <log-manager/LogServerRequest.h>

#include <boost/bind.hpp>
#include <boost/bind/apply.hpp>

#include <iostream>

namespace sf1r
{

#define RETURN_ON_FAILURE(condition)                                        \
if (! (condition))                                                          \
{                                                                           \
    return false;                                                           \
}

#define LOG_SERVER_ASSERT(condition)                                        \
if (! (condition))                                                          \
{                                                                           \
    std::cerr << "Assertion failed: " #condition                            \
              << std::endl;                                                 \
    return false;                                                           \
}


LogServerProcess::LogServerProcess()
{
}

LogServerProcess::~LogServerProcess()
{
}

bool LogServerProcess::init(const std::string& cfgFile)
{
    // Parse config first
    RETURN_ON_FAILURE(LogServerCfg::get()->parse(cfgFile));

    RETURN_ON_FAILURE(LogServerStorage::get()->init());
    RETURN_ON_FAILURE(initRpcLogServer());
    RETURN_ON_FAILURE(initDriverLogServer());

    addExitHook(boost::bind(&LogServerProcess::stop, this));

    return true;
}

void LogServerProcess::start()
{
    LOG(INFO) << "\tRPC Server    : port=" << rpcLogServer_->getPort();
    rpcLogServer_->start();

    LOG(INFO) << "\tDriver Server : port=" << driverLogServer_->getPort();
    driverLogServer_->start();
}

void LogServerProcess::join()
{
    driverLogServer_->join();
}

void LogServerProcess::stop()
{
    if (rpcLogServer_)
    {
        rpcLogServer_->stop();
    }

    if (driverLogServer_)
    {
        driverLogServer_->stop();
    }

    LogServerStorage::get()->close();
    std::cout << "LogServerProcess::stop()" << std::endl;
}

bool LogServerProcess::initRpcLogServer()
{
    rpcLogServer_.reset(
            new RpcLogServer(
                LogServerCfg::get()->getLocalHost(),
                LogServerCfg::get()->getRpcServerPort(),
                LogServerCfg::get()->getRpcThreadNum()
            ));

    if (rpcLogServer_ && rpcLogServer_->init())
    {
        return true;
    }

    return false;
}

bool LogServerProcess::initDriverLogServer()
{
    driverLogServer_.reset(
            new DriverLogServer(
                LogServerCfg::get()->getDriverServerPort(),
                LogServerCfg::get()->getDriverThreadNum()
            ));

    if (driverLogServer_ && driverLogServer_->init())
    {
        return true;
    }

    return false;
}

}

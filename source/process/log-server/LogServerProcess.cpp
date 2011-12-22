#include "LogServerProcess.h"

#include <iostream>

#include <log-manager/LogServerRequest.h>

namespace sf1r
{

#define RETURN_ON_FAILURE(condition)                                        \
if (! (condition))                                                          \
{                                                                           \
    std::cerr << "Log Server initialization assertion failed: " #condition  \
              << std::endl;                                                 \
    return false;                                                           \
}


LogServerProcess::LogServerProcess()
{
}

LogServerProcess::~LogServerProcess()
{
    stop();
}

bool LogServerProcess::init(const std::string& cfgFile)
{
    // Parse config first
    RETURN_ON_FAILURE(logServerCfg_.parse(cfgFile));

    RETURN_ON_FAILURE(initRpcLogServer());
    RETURN_ON_FAILURE(initDriverLogServer());
    RETURN_ON_FAILURE(initDrum());

    return true;
}

void LogServerProcess::start()
{
    LOG(INFO) << "\tRPC Server : port=" << rpcLogServer_->getPort();
    rpcLogServer_->start();

    LOG(INFO) << "\tDriver Server : port=" << driverLogServer_->getPort();
    driverLogServer_->start();
}

void LogServerProcess::join()
{
    rpcLogServer_->join();
    driverLogServer_->join();
}

void LogServerProcess::stop()
{
    if (rpcLogServer_)
        rpcLogServer_->stop();
    if (driverLogServer_)
        driverLogServer_->stop();
}

bool LogServerProcess::initRpcLogServer()
{
    rpcLogServer_.reset(
        new RpcLogServer(
            logServerCfg_.getLocalHost(),
            logServerCfg_.getRpcServerPort(),
            logServerCfg_.getThreadNum()
        ));

    return (rpcLogServer_ != 0);
}

bool LogServerProcess::initDriverLogServer()
{
    driverLogServer_.reset(
        new DriverLogServer(
            logServerCfg_.getDriverServerPort(),
            logServerCfg_.getThreadNum()
        ));

    if (driverLogServer_)
    {
        return driverLogServer_->init();
    }
    return false;
}

bool LogServerProcess::initDrum()
{
    drum_.reset(new DrumType(
                logServerCfg_.getDrumName(),
                logServerCfg_.getDrumNumBuckets(),
                logServerCfg_.getDrumBucketBuffElemSize(),
                logServerCfg_.getDrumBucketByteSize()
                ));

    return drum_;
}

}

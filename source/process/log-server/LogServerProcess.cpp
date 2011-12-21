#include "LogServerProcess.h"

#include <iostream>

#include <log-manager/LogServerRequest.h>

namespace sf1r
{

LogServerProcess::LogServerProcess()
{
}

bool LogServerProcess::init(const std::string& cfgFile)
{
    if (!logServerCfg_.parse(cfgFile))
    {
        return false;
    }

    return initRpcLogServer();
}

void LogServerProcess::start()
{
    LOG(INFO) << "\tRPC Server : port=" << rpcLogServer_->getPort();
    rpcLogServer_->start();
}

void LogServerProcess::join()
{
    rpcLogServer_->join();
}

void LogServerProcess::stop()
{
    rpcLogServer_->stop();
}

bool LogServerProcess::initRpcLogServer()
{
    rpcLogServer_.reset(
            new RpcLogServer(
                logServerCfg_.getLocalHost(),
                logServerCfg_.getServerPort(),
                logServerCfg_.getThreadNum()
            ));

    return true;
}

}


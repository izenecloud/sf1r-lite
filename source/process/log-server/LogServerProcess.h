#ifndef LOG_SERVER_PROCESS_H_
#define LOG_SERVER_PROCESS_H_

#include "LogServerCfg.h"
#include "RpcLogServer.h"
#include "DriverLogServer.h"

#include <OnSignal.h>

#include <glog/logging.h>

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

namespace sf1r
{

class LogServerProcess
{
public:
    LogServerProcess();

    ~LogServerProcess();

    bool init(const std::string& cfgFile);

    /// @brief start all
    void start();

    /// @brief join all
    void join();

    /// @brief interrupt
    void stop();

private:
    bool initRpcLogServer();

    bool initDriverLogServer();

    bool initDrum();

private:
    boost::scoped_ptr<RpcLogServer> rpcLogServer_;
    boost::scoped_ptr<DriverLogServer> driverLogServer_;
};

}

#endif /* LOG_SERVER_PROCESS_H_ */

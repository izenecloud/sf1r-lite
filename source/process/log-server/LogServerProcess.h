#ifndef LOG_SERVER_PROCESS_H_
#define LOG_SERVER_PROCESS_H_

#include "LogServerCfg.h"
#include "RpcLogServer.h"

#include <glog/logging.h>

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

namespace sf1r
{

class LogServerProcess
{
public:
    LogServerProcess();

    bool init(const std::string& cfgFile);

    /// @brief start all
    void start();

    /// @brief join all
    void join();

    /// @brief interrupt
    void stop();

private:
    bool initRpcLogServer();

private:
    LogServerCfg logServerCfg_;

    boost::scoped_ptr<RpcLogServer> rpcLogServer_;
};

}

#endif /* LOG_SERVER_PROCESS_H_ */

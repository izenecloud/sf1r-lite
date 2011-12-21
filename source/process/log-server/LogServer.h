#ifndef LOG_SERVER_H_
#define LOG_SERVER_H_

#include "LogServerCfg.h"

namespace sf1r
{

class LogServer
{
public:
    LogServer();

    bool init(const std::string& cfgFile);

    void start();

    void stop();

private:
    LogServerCfg logServerCfg_;
};

}

#endif /* LOG_SERVER_H_ */

/**
 * @file LogServerCfg.h
 * @author Zhongxia Li
 * @date Dec 20, 2011
 * @brief Log server configuration
 */
#ifndef LOG_SERVER_CFG_H_
#define LOG_SERVER_CFG_H_

#include <string>

namespace sf1r
{

class LogServerCfg
{
public:
    LogServerCfg();

    ~LogServerCfg();

    bool parse(const std::string& cfgFile);

    inline const std::string& getLocalHost() const
    {
        return host_;
    }

    inline unsigned int getRpcServerPort() const
    {
        return rpcPort_;
    }

    inline unsigned int getDriverServerPort() const
    {
        return driverPort_;
    }

    inline unsigned int getThreadNum() const
    {
        return threadNum_;
    }

    inline const std::string& getDrumName() const
    {
        return drum_name_;
    }

private:
    bool parseCfgFile_(const std::string& cfgFile);

private:
    std::string cfgFile_;

    std::string host_;
    unsigned int rpcPort_;
    unsigned int driverPort_;
    unsigned int threadNum_;

    std::string drum_name_;
};

}

#endif /* LOGSERVERCFG_H_ */

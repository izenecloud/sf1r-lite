/**
 * @file WorkerConfig.h
 * @author Zhongxia Li
 * @date Sep 1, 2011
 * @brief 
 */
#ifndef WORKER_CONFIG_H_
#define WORKER_CONFIG_H_

#include <sstream>

namespace sf1r
{

struct WorkerAgentConfig
{
    WorkerAgentConfig() : enabled_(false) {}

    bool enabled_;
    unsigned int port_;

    std::string masterHost_;
    unsigned int masterPort_;

    std::string toString()
    {
        std::stringstream ss;
        ss<<"[WorkerAgentConfig] enabled ? "<<enabled_<<" port : "<<port_<<endl;
        ss<<"Master host "<< masterHost_<<" port "<<masterPort_<<endl;

        return ss.str();
    }
};

}

#endif /* WORKER_CONFIG_H_ */

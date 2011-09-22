#ifndef _BROKER_AGENT_CONFIG_H_
#define _BROKER_AGENT_CONFIG_H_

namespace sf1r
{



struct BrokerAgentConfig
{
    bool useCache_;
    size_t threadNum_;
    bool enableTest_;
    unsigned int port_;
    unsigned int nodeId_;
    unsigned int mirrorId_;
};

}

#endif //  _BROKER_AGENT_CONFIG_H_

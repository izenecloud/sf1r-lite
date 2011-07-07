#ifndef _BROKER_AGENT_CONFIG_H_
#define _BROKER_AGENT_CONFIG_H_

namespace sf1r
{



struct AggregatorConfigUnit
{
    std::string name_;
};

struct BrokerAgentConfig
{
    bool useCache_;
    size_t threadNum_;
    bool enableTest_;
    unsigned int port_;

    std::map<std::string, AggregatorConfigUnit> aggregatorUnitMap_;

    void addAggregatorConfig(const AggregatorConfigUnit& unit)
    {
        aggregatorUnitMap_.insert(
                std::pair<std::string, AggregatorConfigUnit>(unit.name_, unit));
    }
};

}

#endif //  _BROKER_AGENT_CONFIG_H_

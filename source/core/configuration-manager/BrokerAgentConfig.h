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
    unsigned int workerport_;

    std::map<std::string, AggregatorConfigUnit> aggregatorUnitMap_;

    void addAggregatorConfig(const AggregatorConfigUnit& unit)
    {
        aggregatorUnitMap_.insert(
                std::pair<std::string, AggregatorConfigUnit>(unit.name_, unit));
    }

    bool checkAggregatorByName(const std::string& name)
    {
        if (aggregatorUnitMap_.find(name) != aggregatorUnitMap_.end())
        {
            return true;
        }

        return false;
    }
};

}

#endif //  _BROKER_AGENT_CONFIG_H_

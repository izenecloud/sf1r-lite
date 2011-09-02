/**
 * @file MasterAgentConfig.h
 * @author Zhongxia Li
 * @date Sep 1, 2011
 * @brief 
 */
#ifndef MASTER_AGENT_CONFIG_H_
#define MASTER_AGENT_CONFIG_H_

#include <net/aggregator/AggregatorConfig.h>
#include <sstream>

struct AggregatorUnit
{
    std::string name_;
};

class MasterAgentConfig
{
public:
    MasterAgentConfig() : enabled_(false) {}

    void addAggregatorConfig(const AggregatorUnit& unit)
    {
        aggregatorSupportMap_.insert(std::pair<std::string, AggregatorUnit>(unit.name_, unit));
    }

    bool checkAggregatorByName(const std::string& name)
    {
        if (aggregatorSupportMap_.find(name) != aggregatorSupportMap_.end())
        {
            return true;
        }

        return false;
    }

    std::string toString()
    {
        std::stringstream ss;
        ss<<"[MasterAgentConfig] enabled ? "<<enabled_<<" port : "<<port_<<endl;

        std::map<std::string, AggregatorUnit>::iterator it;
        for (it = aggregatorSupportMap_.begin(); it != aggregatorSupportMap_.end(); it++)
        {
            ss<<"Aggregator="<<it->first<<endl;
        }

        ss << aggregatorConfig_.toString();

        return ss.str();
    }

public:
    bool enabled_;
    unsigned int port_;
    std::map<std::string, AggregatorUnit> aggregatorSupportMap_;

    net::aggregator::AggregatorConfig aggregatorConfig_;
};

#endif /* MASTERAGENTCONFIG_H_ */

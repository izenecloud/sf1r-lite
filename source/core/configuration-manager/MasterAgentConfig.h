/**
 * @file MasterAgentConfig.h
 * @author Zhongxia Li
 * @date Sep 1, 2011
 * @brief 
 */
#ifndef MASTER_AGENT_CONFIG_H_
#define MASTER_AGENT_CONFIG_H_


#include <sstream>

struct AggregatorUnit
{
    std::string name_;
};

class MasterAgentConfig
{
public:
    MasterAgentConfig()
        : enabled_(false)
        , shardNum_(0)
    {}

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
        ss<<"[MasterAgentConfig] " << (enabled_ ? "enabled":"disabled") << ", workerNum:" << shardNum_ <<std::endl;

        ss << "collections: ";
        std::vector<std::string>::iterator sit;
        for (sit = collectionList_.begin(); sit != collectionList_.end(); sit++)
        {
            ss << " " << *sit;
        }
        ss << std::endl;

        std::map<std::string, AggregatorUnit>::iterator it;
        for (it = aggregatorSupportMap_.begin(); it != aggregatorSupportMap_.end(); it++)
        {
            ss<<"Aggregator="<<it->first<<std::endl;
        }

        return ss.str();
    }

public:
    bool enabled_;
    std::string name_;
    unsigned int shardNum_;
    std::vector<unsigned int> shardIdList_;
    std::vector<std::string> collectionList_;
    std::map<std::string, AggregatorUnit> aggregatorSupportMap_;
};

#endif /* MASTERAGENTCONFIG_H_ */

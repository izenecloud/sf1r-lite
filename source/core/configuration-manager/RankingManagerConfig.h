
#ifndef _RANKING_MANAGER_CONFIG_H_
#define _RANKING_MANAGER_CONFIG_H_

#include <configuration-manager/RankingConfigUnit.h>

namespace sf1r
{

struct RankingManagerConfig
{
public:
    RankingConfigUnit rankingConfigUnit_;

    // map<property name, weight of property>
    std::map<std::string, float> propertyWeightMapByProperty_;
};

}

#endif  //_RANKING_MANAGER_CONFIG_H_


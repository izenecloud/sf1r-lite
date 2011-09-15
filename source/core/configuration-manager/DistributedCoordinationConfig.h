/**
 * @file DistributedCoordinationConfig.h
 * @author Zhongxia Li
 * @date Sep 14, 2011
 * @brief 
 */
#ifndef DISTRIBUTED_COORDINATION_CONFIG_H_
#define DISTRIBUTED_COORDINATION_CONFIG_H_

namespace sf1r {

class DistributedCoordinationConfig
{
public:
    std::string zkHosts_;
    unsigned int zkRecvTimeout_;
};

}

#endif /* DISTRIBUTED_COORDINATION_CONFIG_H_ */

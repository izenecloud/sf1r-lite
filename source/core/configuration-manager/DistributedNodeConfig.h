/**
 * @file DistributedNodeConfig.h
 * @brief configuration of distributed node deployment
 * @author Jun Jiang
 * @date 2012-03-13
 */

#ifndef DISTRIBUTED_NODE_CONFIG_H
#define DISTRIBUTED_NODE_CONFIG_H

namespace sf1r
{

struct DistributedNodeConfig
{
    DistributedNodeConfig()
        : isSingleNode_(false)
        , isMasterNode_(false)
        , isWorkerNode_(false)
    {}

    /**
     * if the bundle is not activated,
     * all below members should be false.
     */
    bool isSingleNode_; /// not distributed
    bool isMasterNode_;
    bool isWorkerNode_;
};

} // namespace sf1r

#endif // DISTRIBUTED_NODE_CONFIG_H

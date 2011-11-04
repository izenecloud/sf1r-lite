/**
 * @file ScdDispatcher.h
 * @author Zhongxia Li
 * @date Nov 3, 2011
 * @brief Sharding (partitioning) scd files and dispatching to different shards.
 */
#ifndef SCD_DISPATCHER_H_
#define SCD_DISPATCHER_H_

#include "ScdSharding.h"

#include <common/ScdParser.h>

namespace sf1r{


class DispatchAction
{
public:
    virtual void init(ShardingConfig& shardConfig) {};
    virtual void dispatch(shardid_t shardid, SCDDoc& scdDoc) = 0;
    virtual void finish() {}
};

class DispatchActionToFile : public DispatchAction
{
public:
    virtual void init(ShardingConfig& shardConfig);
    virtual void dispatch(shardid_t shardid, SCDDoc& scdDoc);
    virtual void finish();

private:
    std::map<shardid_t, std::string> subFiles_;
};

class ScdDispatcher
{
public:
    ScdDispatcher(ScdSharding* scdSharding, DispatchAction* dispatchAction);

    /**
     * Sharding & dispatching
     * @param dir directory containing scd files
     * @param docNum number of docs to be dispatched, 0 for all
     */
    bool dispatch(const std::string& dir, unsigned int docNum = 0);

private:
    bool getScdFileList(const std::string& dir, std::vector<std::string>& fileList);

private:
    ScdSharding* scdSharding_;
    DispatchAction* dispatchAction_;

    izenelib::util::UString::EncodingType scdEncoding_;
};

}

#endif /* SCD_DISPATCHER_H_ */

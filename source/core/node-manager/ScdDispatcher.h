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

#include <iostream>

#include <boost/shared_ptr.hpp>

namespace sf1r{

class ScdDispatcher
{
public:
    ScdDispatcher(ScdSharding* scdSharding);

    virtual ~ScdDispatcher() {}

    /**
     * Sharding & dispatching
     * @param dir directory containing scd files
     * @param docNum number of docs to be dispatched, 0 for all
     */
    bool dispatch(const std::string& dir, unsigned int docNum = 0);

protected:
    bool getScdFileList(const std::string& dir, std::vector<std::string>& fileList);

    virtual bool dispatch_impl(shardid_t shardid, SCDDoc& scdDoc) = 0;

    virtual void finish() {}

protected:
    ScdSharding* scdSharding_;

    izenelib::util::UString::EncodingType scdEncoding_;
    std::string curScdFileName_;
};

/**
 * SCD will be splitted into sub files according shard id,
 * and then be sent to each shard server.
 */
class BatchScdDispatcher : public ScdDispatcher
{
public:
    BatchScdDispatcher(ScdSharding* scdSharding);

    ~BatchScdDispatcher();

protected:
    virtual bool dispatch_impl(shardid_t shardid, SCDDoc& scdDoc);

    virtual void finish();

private:
    std::string dispatchTempDir_;
    std::map<shardid_t, std::string> shardScdfileMap_;
    std::vector<std::ofstream*> ofList_;
};

/**
 * Each SCD doc will be dispatching instantly after sharding (got shard id).
 */
class InstantScdDispatcher : public ScdDispatcher
{
public:
    ;

protected:
    virtual bool dispatch_impl(shardid_t shardid, SCDDoc& scdDoc);
};

}

#endif /* SCD_DISPATCHER_H_ */

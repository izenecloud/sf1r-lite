/**
 * @file ScdDispatcher.h
 * @author Zhongxia Li
 * @date Nov 3, 2011
 * @brief Sharding (partitioning) scd files and dispatching to different shards.
 */
#ifndef SCD_DISPATCHER_H_
#define SCD_DISPATCHER_H_

#include "ScdSharding.h"

#include <net/aggregator/AggregatorConfig.h>

#include <common/ScdParser.h>

#include <iostream>

#include <boost/shared_ptr.hpp>

using namespace net::aggregator;

namespace sf1r{

class ScdDispatcher
{
public:
    ScdDispatcher(ScdSharding* scdSharding, AggregatorConfig& aggregatorConfig);

    virtual ~ScdDispatcher() {}

    /**
     * Sharding & dispatching
     * @param dir directory containing scd files
     * @param docNum number of docs to be dispatched, 0 for all
     */
    bool dispatch(const std::string& dir, unsigned int docNum = 0);

protected:
    bool getScdFileList(const std::string& dir, std::vector<std::string>& fileList);

    virtual bool switchFile() { return true; }

    virtual bool dispatch_impl(shardid_t shardid, SCDDoc& scdDoc) = 0;

    virtual bool finish() { return true; }

protected:
    ScdSharding* scdSharding_;
    AggregatorConfig aggregatorConfig_;

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
    BatchScdDispatcher(
            ScdSharding* scdSharding,
            AggregatorConfig& aggregatorConfig,
            const std::string& collectionName,
            const std::string& dispatchTempDir="./scd-dispatch-temp");

    ~BatchScdDispatcher();

protected:
    virtual bool switchFile();

    virtual bool dispatch_impl(shardid_t shardid, SCDDoc& scdDoc);

    virtual bool finish();

private:
    std::string collectionName_;
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

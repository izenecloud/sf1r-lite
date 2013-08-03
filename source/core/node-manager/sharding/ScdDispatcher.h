/**
 * @file ScdDispatcher.h
 * @author Zhongxia Li
 * @date Nov 3, 2011
 * @brief Sharding (partitioning) scd files and dispatching to different shards.
 */
#ifndef SCD_DISPATCHER_H_
#define SCD_DISPATCHER_H_

#include "ScdSharder.h"

#include <net/aggregator/AggregatorConfig.h>

#include <common/ScdParser.h>

#include <iostream>

#include <boost/shared_ptr.hpp>

using namespace net::aggregator;

namespace sf1r{

class ScdDispatcher
{
public:
    ScdDispatcher(const boost::shared_ptr<ScdSharder>& scdSharder);

    virtual ~ScdDispatcher() {}

    /**
     * @param outScdFileList  [OUT] scd file list
     * @param dir  scd directory
     * @param docNum  number of docs to be dispatched, 0 for all
     * @return
     */
    bool dispatch(std::vector<std::string>& scdFileList, const std::string& dir,
        const std::string& to_dir, unsigned int docNum = 0);

protected:
    bool getScdFileList(const std::string& dir, std::vector<std::string>& fileList);

    virtual bool initialize() { return true; }

    virtual bool switchFile() { return true; }

    virtual bool dispatch_impl(shardid_t shardid, SCDDoc& scdDoc) = 0;

    virtual bool finish() { return true; }

protected:
    boost::shared_ptr<ScdSharder> scdSharder_;

    std::string scdDir_;
    izenelib::util::UString::EncodingType scdEncoding_;
    std::string curScdFileName_;
    std::string to_scdDir_;
};

/**
 * SCD will be splitted into sub files according shard id,
 * and then be sent to each shard server.
 */
class BatchScdDispatcher : public ScdDispatcher
{
public:
    BatchScdDispatcher(
            const boost::shared_ptr<ScdSharder>& scdSharder,
            const std::string& collectionName, bool is_dfs_enabled);

    ~BatchScdDispatcher();

protected:
    virtual bool initialize();

    virtual bool switchFile();

    virtual bool dispatch_impl(shardid_t shardid, SCDDoc& scdDoc);

    virtual bool finish();

private:
    bool initTempDir(const std::string& tempDir);

private:
    std::string service_;
    std::string collectionName_;
    bool is_dfs_enabled_;
    std::string dispatchTempDir_;
    std::map<shardid_t, std::string> shardScdfileMap_;
    std::map<shardid_t, std::ofstream*> ofList_;
};

/**
 * Each SCD doc will be dispatching instantly after sharding (got shard id).
 */
class InstantScdDispatcher : public ScdDispatcher
{
protected:
    virtual bool dispatch_impl(shardid_t shardid, SCDDoc& scdDoc);
};

}

#endif /* SCD_DISPATCHER_H_ */

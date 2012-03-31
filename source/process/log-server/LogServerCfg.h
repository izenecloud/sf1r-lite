/**
 * @file LogServerCfg.h
 * @author Zhongxia Li
 * @date Dec 20, 2011
 * @brief Log server configuration
 */
#ifndef LOG_SERVER_CFG_H_
#define LOG_SERVER_CFG_H_

#include <string>
#include <set>

#include <util/singleton.h>
#include <util/kv2string.h>

namespace sf1r
{

typedef izenelib::util::kv2string properties;

class LogServerCfg
{
public:
    enum DrumType
    {
        UUID,
        DOCID
    };

public:
    LogServerCfg();

    ~LogServerCfg();

    static LogServerCfg* get()
    {
        return izenelib::util::Singleton<LogServerCfg>::get();
    }

    bool parse(const std::string& cfgFile);

    inline const std::string& getLocalHost() const
    {
        return host_;
    }

    inline unsigned int getRpcServerPort() const
    {
        return rpcPort_;
    }

    inline unsigned int getRpcThreadNum() const
    {
        return rpcThreadNum_;
    }

    inline unsigned int getRpcRequestQueueSize() const
    {
        return rpcRequestQueueSize_;
    }

    inline unsigned int getDriverServerPort() const
    {
        return driverPort_;
    }

    inline unsigned int getDriverThreadNum() const
    {
        return driverThreadNum_;
    }

    const std::set<std::string>& getDriverCollections() const
    {
        return driverCollections_;
    }

    inline const std::string& getStorageBaseDir() const
    {
        return base_dir_;
    }

    inline unsigned int getFlushCheckInterval() const
    {
        return flush_check_interval_;
    }

    inline const std::string& getDrumName(DrumType type) const
    {
        if (type == UUID)
            return uuid_drum_name_;
        else //if (type == DOCID)
            return docid_drum_name_;
    }

    inline unsigned int getDrumNumBuckets(DrumType type) const
    {
        if (type == UUID)
            return uuid_drum_num_buckets_;
        else //if (type == DOCID)
            return docid_drum_num_buckets_;
    }

    inline unsigned int getDrumBucketBuffElemSize(DrumType type) const
    {
        if (type == UUID)
            return uuid_drum_bucket_buff_elem_size_;
        else //if (type == DOCID)
            return docid_drum_bucket_buff_elem_size_;
    }

    inline unsigned int getDrumBucketByteSize(DrumType type) const
    {
        if (type == UUID)
            return uuid_drum_bucket_byte_size_;
        else //if (type == DOCID)
            return docid_drum_bucket_byte_size_;
    }


private:
    bool parseCfgFile_(const std::string& cfgFile);

    void parseCfg(properties& props);

    void parseServerCfg(properties& props);

    void parseStorageCfg(properties& props);

    void parseDriverCollections(const std::string& collections);

private:
    std::string cfgFile_;

    std::string host_;

    unsigned int rpcPort_;
    unsigned int rpcThreadNum_;
    unsigned int rpcRequestQueueSize_;

    unsigned int driverPort_;
    unsigned int driverThreadNum_;
    std::set<std::string> driverCollections_;

    std::string base_dir_;
    unsigned int flush_check_interval_;

    std::string  uuid_drum_name_;
    unsigned int uuid_drum_num_buckets_;
    unsigned int uuid_drum_bucket_buff_elem_size_;
    unsigned int uuid_drum_bucket_byte_size_;

    std::string  docid_drum_name_;
    unsigned int docid_drum_num_buckets_;
    unsigned int docid_drum_bucket_buff_elem_size_;
    unsigned int docid_drum_bucket_byte_size_;
};

}

#endif /* LOGSERVERCFG_H_ */

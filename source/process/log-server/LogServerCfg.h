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

    inline const std::string& getDrumName() const
    {
        return drum_name_;
    }

    inline unsigned int getDrumNumBuckets() const
    {
        return drum_num_buckets_;
    }

    inline unsigned int getDrumBucketBuffElemSize() const
    {
        return drum_bucket_buff_elem_size_;
    }

    inline unsigned int getDrumBucketByteSize() const
    {
        return drum_bucket_byte_size_;
    }

    inline const std::string& getDocidDBName() const
    {
        return docid_db_name_;
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
    unsigned int driverPort_;
    unsigned int driverThreadNum_;
    std::set<std::string> driverCollections_;

    std::string base_dir_;

    std::string drum_name_;
    unsigned int drum_num_buckets_;
    unsigned int drum_bucket_buff_elem_size_;
    unsigned int drum_bucket_byte_size_;

    std::string docid_db_name_;
};

}

#endif /* LOGSERVERCFG_H_ */

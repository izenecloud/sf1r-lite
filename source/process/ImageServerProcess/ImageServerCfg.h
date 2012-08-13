/**
 * @file ImageServerCfg.h
 * @author Zhongxia Li
 * @date Dec 20, 2011
 * @brief Color server configuration
 */
#ifndef COLOR_SERVER_CFG_H_
#define COLOR_SERVER_CFG_H_

#include <string>
#include <set>

#include <util/singleton.h>
#include <util/kv2string.h>

namespace sf1r
{

typedef izenelib::util::kv2string properties;

class ImageServerCfg
{
public:
    ImageServerCfg();

    ~ImageServerCfg();

    static ImageServerCfg* get()
    {
        return izenelib::util::Singleton<ImageServerCfg>::get();
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

    inline const std::string& getStorageBaseDir() const
    {
        return base_dir_;
    }

    inline std::string getImageColorDB() const
    {
        return base_dir_ + "/" + img_color_dbname_;
    }

    inline const std::string& getImageFilePath() const
    {
        return img_file_fullpath_;
    }

    inline const std::string& getSCDFilePath() const
    {
        return scd_file_fullpath_;
    }

    inline unsigned int getComputeThread() const
    {
        return compute_thread_;
    }

    inline const std::string& getImageFileServer() const
    {
        return img_file_server_;
    }

    inline const std::string& getUploadImageLog() const
    {
        return img_upload_log_;
    }

private:
    bool parseCfgFile_(const std::string& cfgFile);

    void parseCfg(properties& props);

    void parseServerCfg(properties& props);

    void parseStorageCfg(properties& props);

private:
    std::string cfgFile_;

    std::string host_;

    unsigned int rpcPort_;
    unsigned int rpcThreadNum_;
    unsigned int rpcRequestQueueSize_;
    unsigned int compute_thread_;

    std::string base_dir_;
    std::string img_color_dbname_;
    std::string scd_file_fullpath_;
    std::string img_file_fullpath_;
    std::string img_file_server_;
    std::string img_upload_log_;
};

}

#endif /* COLORSERVERCFG_H_ */

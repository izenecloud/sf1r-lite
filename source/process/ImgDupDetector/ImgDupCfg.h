/**
 * @file ImgDupCfg.h
 * @author Kuilong Liu
 * @date Dec 18, 2012
 * @brief ImgDupDetector configuration
 */
#ifndef IMG_DUP_CFG_H_
#define IMG_DUP_CFG_H_

#include <string>
#include <set>

#include <util/singleton.h>
#include <util/kv2string.h>

typedef izenelib::util::kv2string properties;

class ImgDupCfg
{
public:
    ImgDupCfg();

    ~ImgDupCfg();

    static ImgDupCfg* get()
    {
        return izenelib::util::Singleton<ImgDupCfg>::get();
    }

    bool parse(const std::string& cfgFile);

    inline const std::string& getLocalHost() const
    {
        return host_;
    }


private:
    bool parseCfgFile_(const std::string& cfgFile);

    void parseCfg(properties& props);

    void parseServerCfg(properties& props);


public:
    std::string cfgFile_;

    std::string host_;

    std::string scd_input_dir_;

    std::string scd_output_dir_;

    std::string source_name_;

    std::string incremental_mode_;

    std::string detect_by_imgurl_;

    std::string detect_by_imgcontent_;

};

#endif /* IMGDUPCFG_H_ */

#ifndef SF1R_B5MMANAGER_B5MM_H_
#define SF1R_B5MMANAGER_B5MM_H_

#include <3rdparty/yaml-cpp/yaml.h>
#include <boost/filesystem.hpp>

NS_SF1R_B5M_BEGIN
struct B5mM
{
    B5mM() 
    {
    }
    B5mM(const std::string& m)
    {
        SetDefaultValue_(m);
        Load(m);
    }
    bool Load(const std::string& m)
    {
        path = m;
        std::string m_config_file = m+"/config";
        if(!boost::filesystem::exists(m_config_file)) return false;
        YAML::Node config = YAML::LoadFile(m_config_file);
        config = config["config"];
        mobile_source = config["path_of"]["mobile_source"].as<std::string>();
        human_match = config["path_of"]["human_match"].as<std::string>();
        YAML::Node indexer = config["indexer"];
        if(indexer["type"].as<std::string>()=="hdfs")
        {
            std::string hdfs_dir = indexer["hdfs_mnt"].as<std::string>()+"/"+indexer["hdfs_prefix"].as<std::string>()+"/"+ts;
            b5mo_path = hdfs_dir+"/b5mo";
            b5mp_path = hdfs_dir+"/b5mp";
            b5mc_path = hdfs_dir+"/b5mc";
        }
        return true;
    }
    void Show()
    {
        std::cerr<<"path: "<<path<<std::endl;
        std::cerr<<"ts: "<<ts<<std::endl;
        std::cerr<<"b5mo_path: "<<b5mo_path<<std::endl;
        std::cerr<<"b5mp_path: "<<b5mp_path<<std::endl;
        std::cerr<<"b5mc_path: "<<b5mc_path<<std::endl;
        std::cerr<<"mobile_source: "<<mobile_source<<std::endl;
        std::cerr<<"human_match: "<<human_match<<std::endl;
    }

    std::string path;
    std::string ts;
    std::string b5mo_path;
    std::string b5mp_path;
    std::string b5mc_path;
    std::string mobile_source;
    std::string human_match;
private:
    void SetDefaultValue_(const std::string& m)
    {
        path = m;
        b5mo_path = m+"/b5mo";
        b5mp_path = m+"/b5mp";
        b5mc_path = m+"/b5mc";
        ts = boost::filesystem::path(m).filename().string();
        if(ts==".")
        {
            ts = boost::filesystem::path(m).parent_path().filename().string();
        }
    }

};
NS_SF1R_B5M_END

#endif


#ifndef SF1R_B5MMANAGER_B5MM_H_
#define SF1R_B5MMANAGER_B5MM_H_

#include <3rdparty/yaml-cpp/yaml.h>
#include <boost/filesystem.hpp>

NS_SF1R_B5M_BEGIN
struct B5mM
{
    B5mM(): mode(0), cmode(-1), thread_num(1)
    {
    }
    B5mM(const std::string& m)
    {
        SetDefaultValue_(m);
        Load(m);
    }
    bool Load(const std::string& m)
    {
        SetDefaultValue_(m);
        path = m;
        std::string m_config_file = m+"/config";
        if(!boost::filesystem::exists(m_config_file)) return false;
        YAML::Node config = YAML::LoadFile(m_config_file);
        if(config.Type()==YAML::NodeType::Undefined) return false;
        YAML::Node rconfig = config["config"];
        if(rconfig.Type()==YAML::NodeType::Undefined)
        {
            rconfig = config[":config"];
        }
        if(rconfig.Type()==YAML::NodeType::Undefined)
        {
            rconfig = config;
        }
        config = rconfig;
        if(config.Type()==YAML::NodeType::Undefined) return false;
        YAML::Node path_of = config["path_of"];
        if(path_of.Type()==YAML::NodeType::Undefined) return false;
        YAML::Node indexer = config["indexer"];
        if(indexer.Type()==YAML::NodeType::Undefined) return false;
        SetValue_(path_of["scd"], scd_path);
        SetValue_(path_of["comment_scd"], comment_scd_path);
        SetValue_(path_of["mobile_source"], mobile_source);
        SetValue_(path_of["human_match"], human_match);
        SetValue_(path_of["knowledge"], knowledge);
        SetValue_(path_of["cma"], cma_path);
        if(indexer["type"].as<std::string>()=="hdfs")
        {
            std::string cname = config["schema"].as<std::string>();
            if(indexer["collection_name"].Type()!=YAML::NodeType::Undefined)
            {
                cname = indexer["collection_name"].as<std::string>();
            }
            std::string hdfs_dir = indexer["hdfs_mnt"].as<std::string>()+"/"+indexer["hdfs_prefix"].as<std::string>()+"/"+cname+"/"+ts;
            if(cname=="tuan")
            {
                b5mo_path = hdfs_dir+"/"+cname+"m";
                b5mp_path = hdfs_dir+"/"+cname+"a";
                b5mc_path = hdfs_dir+"/"+cname+"c";
            }
            else
            {
                b5mo_path = hdfs_dir+"/"+cname+"o";
                b5mp_path = hdfs_dir+"/"+cname+"p";
                b5mc_path = hdfs_dir+"/"+cname+"c";
            }
        }
        SetValue_(config["mode"], mode);
        SetValue_(config["cmode"], cmode);
        SetValue_(config["thread_num"], thread_num);
        SetValue_(config["buffer_size"], buffer_size);
        SetValue_(config["sorter_bin"], sorter_bin);
        return true;
    }
    void Gen()
    {
        SetDefaultValue_(path);
    }
    void Show()
    {
        std::cerr<<"path: "<<path<<std::endl;
        std::cerr<<"ts: "<<ts<<std::endl;
        std::cerr<<"mode: "<<mode<<std::endl;
        std::cerr<<"cmode: "<<cmode<<std::endl;
        std::cerr<<"thread_num: "<<thread_num<<std::endl;
        std::cerr<<"b5mo_path: "<<b5mo_path<<std::endl;
        std::cerr<<"b5mp_path: "<<b5mp_path<<std::endl;
        std::cerr<<"b5mc_path: "<<b5mc_path<<std::endl;
        std::cerr<<"scd_path: "<<scd_path<<std::endl;
        std::cerr<<"comment_scd_path: "<<comment_scd_path<<std::endl;
        std::cerr<<"knowledge: "<<knowledge<<std::endl;
        std::cerr<<"cma_path: "<<cma_path<<std::endl;
        std::cerr<<"mobile_source: "<<mobile_source<<std::endl;
        std::cerr<<"human_match: "<<human_match<<std::endl;
        std::cerr<<"buffer_size: "<<buffer_size<<std::endl;
        std::cerr<<"sorter_bin: "<<sorter_bin<<std::endl;
    }

    std::string path;
    std::string ts;
    std::string b5mo_path;
    std::string b5mp_path;
    std::string b5mc_path;
    std::string scd_path;
    std::string comment_scd_path;
    int mode;
    int cmode;
    int thread_num;
    std::string mobile_source;
    std::string human_match;
    std::string knowledge;
    std::string cma_path;
    std::string buffer_size;
    std::string sorter_bin;
private:
    template <typename T>
    bool SetValue_(const YAML::Node& node, T& value)
    {
        if(node.Type()==YAML::NodeType::Undefined) return false;
        value = node.as<T>();
        return true;
    }
    void ShowNode_(const YAML::Node& node)
    {
        for(YAML::const_iterator it = node.begin();it!=node.end();++it)
        {
            std::cout<<it->first.as<std::string>()<<" is "<<it->second.Type()<<std::endl;
        }
    }
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


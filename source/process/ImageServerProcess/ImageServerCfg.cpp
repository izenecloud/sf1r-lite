#include "ImageServerCfg.h"
#include <util/string/StringUtils.h>
#include <iostream>
#include <fstream>
#include <boost/tokenizer.hpp>
#include <boost/filesystem.hpp>

namespace bfs = boost::filesystem;

namespace sf1r
{

static const char* DEFAULT_STORAGE_BASE_DIR = "color_server_storage";

static const unsigned int DEFAULT_RPC_REQUEST_QUEUE_SIZE = 32768;

static const unsigned int DEFAULT_THREAD_NUM = 30;

static const unsigned int MAX_THREAD_NUM = 1024;

ImageServerCfg::ImageServerCfg()
    : rpcPort_(0)
{
}

ImageServerCfg::~ImageServerCfg()
{
}

bool ImageServerCfg::parse(const std::string& cfgFile)
{
    cfgFile_ = cfgFile;
    return parseCfgFile_(cfgFile);
}

bool ImageServerCfg::parseCfgFile_(const std::string& cfgFile)
{
    try
    {
        if (!bfs::exists(cfgFile) || !bfs::is_regular_file(cfgFile))
        {
            std::cerr <<"\""<<cfgFile<< "\" is not existed or not a regular file." << std::endl;
            return false;
        }

        // load configuration file
        std::ifstream cfgInput(cfgFile.c_str());
        std::string cfgString;
        std::string line;

        if (!cfgInput.is_open())
        {
            std::cerr << "Could not open file: " << cfgFile << std::endl;
            return false;
        }

        while (getline(cfgInput, line))
        {
            izenelib::util::Trim(line);
            if (line.empty() || line[0] == '#')
            {
                // ignore empty line and comment line
                continue;
            }

            if (!cfgString.empty())
            {
                cfgString.push_back('\n');
            }
            cfgString.append(line);
        }

        // check configuration properties
        properties props('\n', '=');
        props.loadKvString(cfgString);

        parseCfg(props);
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return false;
    }

    return true;
}

void ImageServerCfg::parseCfg(properties& props)
{
    parseServerCfg(props);
    parseStorageCfg(props);
}

void ImageServerCfg::parseServerCfg(properties& props)
{
    if (!props.getValue("host", host_))
    {
        host_ = "localhost";
    }

    if (!props.getValue("process.computethread", compute_thread_))
    {
        compute_thread_ = 4;
    }

    // rpc server
    if (!props.getValue("rpc.port", rpcPort_))
    {
        throw std::runtime_error("Color Server Configuration missing property: rpc.port");
    }

    if (!props.getValue("rpc.thread_num", rpcThreadNum_))
    {
        rpcThreadNum_ = DEFAULT_THREAD_NUM;
    }
    else
    {
        rpcThreadNum_ = std::min(MAX_THREAD_NUM, rpcThreadNum_);
    }

    if (!props.getValue("rpc.request_queue_size", rpcRequestQueueSize_))
    {
        rpcRequestQueueSize_ = DEFAULT_RPC_REQUEST_QUEUE_SIZE;
    }

    if (!props.getValue("image_fileserver.ns", img_file_server_))
    {
        throw std::runtime_error("Image File Server Configuration mssing property: image_fileserver.ns");
    }
    if (!props.getValue("image_upload_log", img_upload_log_))
    {
        throw std::runtime_error("Image File Server Configuration mssing property: image_upload_log");
    }

}

void ImageServerCfg::parseStorageCfg(properties& props)
{
    // base
    if (!props.getValue("storage.base_dir", base_dir_))
    {
        base_dir_ = DEFAULT_STORAGE_BASE_DIR;
    }
    if (!bfs::exists(base_dir_))
    {
        bfs::create_directories(base_dir_);
    }

    if (!props.getValue("storage.img_color.dbname", img_color_dbname_))
    {
        throw std::runtime_error("Color Server Configuration missing property: storage.img_color.dbname");
    }

    if (!props.getValue("storage.img_file_fullpath", img_file_fullpath_))
    {
        throw std::runtime_error("Color Server Configuration missing property: storage.img_file_fullpath");
    }
    if(img_file_fullpath_.empty() || !bfs::exists(img_file_fullpath_))
    {
        //throw std::runtime_error("Color Server : the image file path did not exist!");
        std::cout << "img_file_fullpath is empty or not exists, use the tfs server instead!" << std::endl;
        img_file_fullpath_.clear();
    }
    props.getValue("storage.scd_file_fullpath", scd_file_fullpath_);
}

}

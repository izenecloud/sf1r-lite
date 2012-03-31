#include "LogServerCfg.h"

#include <util/string/StringUtils.h>

#include <iostream>
#include <fstream>

#include <boost/tokenizer.hpp>
#include <boost/filesystem.hpp>

namespace bfs = boost::filesystem;

namespace sf1r
{

static const char* DEFAULT_STORAGE_BASE_DIR = "log_server_storage";

static const unsigned int DEFAULT_RPC_REQUEST_QUEUE_SIZE = 32768;
static const unsigned int DEFAULT_FLASH_CHECK_INTERVAL = 60; // seconds

static const unsigned int DEFAULT_THREAD_NUM = 30;
static const unsigned int DEFAULT_DRUM_NUM_BUCKETS = 64;
static const unsigned int DEFAULT_DRUM_BUCKET_BUFF_ELEM_SIZE = 8192;
static const unsigned int DEFAULT_DRUM_BUCKET_BYTE_SIZE = 1048576;

static const unsigned int MAX_THREAD_NUM = 1024;
static const unsigned int MAX_DRUM_NUM_BUCKETS = 65536;
static const unsigned int MAX_DRUM_BUCKET_BUFF_ELEM_SIZE = 262144;
static const unsigned int MAX_DRUM_BUCKET_BYTE_SIZE = 67108864;

LogServerCfg::LogServerCfg()
    : rpcPort_(0)
    , driverPort_(0)
{
}

LogServerCfg::~LogServerCfg()
{
}

bool LogServerCfg::parse(const std::string& cfgFile)
{
    cfgFile_ = cfgFile;
    return parseCfgFile_(cfgFile);
}

bool LogServerCfg::parseCfgFile_(const std::string& cfgFile)
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

void LogServerCfg::parseCfg(properties& props)
{
    parseServerCfg(props);
    parseStorageCfg(props);
}

void LogServerCfg::parseServerCfg(properties& props)
{
    if (!props.getValue("host", host_))
    {
        host_ = "localhost";
    }

    // rpc server
    if (!props.getValue("rpc.port", rpcPort_))
    {
        throw std::runtime_error("Log Server Configuration missing property: rpc.port");
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

    // driver server
    if (!props.getValue("driver.port", driverPort_))
    {
        throw std::runtime_error("Log Server Configuration missing property: driver.port");
    }

    if (!props.getValue("driver.thread_num", driverThreadNum_))
    {
        driverThreadNum_ = DEFAULT_THREAD_NUM;
    }
    else
    {
        driverThreadNum_ = std::min(MAX_THREAD_NUM, driverThreadNum_);
    }

    std::string collections;
    if (props.getValue("driver.collections", collections))
    {
        parseDriverCollections(collections);
    }
}

void LogServerCfg::parseStorageCfg(properties& props)
{
    // base
    if (!props.getValue("storage.base_dir", base_dir_))
    {
        base_dir_ = DEFAULT_STORAGE_BASE_DIR;
    }
    if (!bfs::exists(base_dir_))
    {
        bfs::create_directories(base_dir_);
        bfs::create_directories(base_dir_+"/scd");
        bfs::create_directories(base_dir_+"/cclog");
    }

    if (!props.getValue("storage.flush_check_interval", flush_check_interval_))
    {
        flush_check_interval_ = DEFAULT_FLASH_CHECK_INTERVAL;
    }

    // uuid drum
    if (!props.getValue("storage.uuid_drum.name", uuid_drum_name_))
    {
        throw std::runtime_error("Log Server Configuration missing property: storage.uuid_drum.name");
    }

    uuid_drum_name_ = base_dir_ + "/" + uuid_drum_name_;
    if (!bfs::exists(uuid_drum_name_))
    {
        bfs::create_directories(uuid_drum_name_);
    }

    if (!props.getValue("storage.uuid_drum.num_buckets", uuid_drum_num_buckets_))
    {
        uuid_drum_num_buckets_ = DEFAULT_DRUM_NUM_BUCKETS;
    }
    else
    {
        uuid_drum_num_buckets_ = std::min(MAX_DRUM_NUM_BUCKETS, uuid_drum_num_buckets_);
    }

    if (!props.getValue("storage.uuid_drum.bucket_buff_elem_size", uuid_drum_bucket_buff_elem_size_))
    {
        uuid_drum_bucket_buff_elem_size_ = DEFAULT_DRUM_BUCKET_BUFF_ELEM_SIZE;
    }
    else
    {
        uuid_drum_bucket_buff_elem_size_ = std::min(MAX_DRUM_BUCKET_BUFF_ELEM_SIZE, uuid_drum_bucket_buff_elem_size_);
    }

    if (!props.getValue("storage.uuid_drum.bucket_byte_size", uuid_drum_bucket_byte_size_))
    {
        uuid_drum_bucket_byte_size_ = DEFAULT_DRUM_BUCKET_BYTE_SIZE;
    }
    else
    {
        uuid_drum_bucket_byte_size_ = std::min(MAX_DRUM_BUCKET_BYTE_SIZE, uuid_drum_bucket_byte_size_);
    }

    // docid drum
    if (!props.getValue("storage.docid_drum.name", docid_drum_name_))
    {
        throw std::runtime_error("Log Server Configuration missing property: storage.docid_drum.name");
    }

    docid_drum_name_ = base_dir_ + "/" + docid_drum_name_;
    if (!bfs::exists(docid_drum_name_))
    {
        bfs::create_directories(docid_drum_name_);
    }

    if (!props.getValue("storage.docid_drum.num_buckets", docid_drum_num_buckets_))
    {
        docid_drum_num_buckets_ = DEFAULT_DRUM_NUM_BUCKETS;
    }
    else
    {
        docid_drum_num_buckets_ = std::min(MAX_DRUM_NUM_BUCKETS, docid_drum_num_buckets_);
    }

    if (!props.getValue("storage.docid_drum.bucket_buff_elem_size", docid_drum_bucket_buff_elem_size_))
    {
        docid_drum_bucket_buff_elem_size_ = DEFAULT_DRUM_BUCKET_BUFF_ELEM_SIZE;
    }
    else
    {
        docid_drum_bucket_buff_elem_size_ = std::min(MAX_DRUM_BUCKET_BUFF_ELEM_SIZE, docid_drum_bucket_buff_elem_size_);
    }

    if (!props.getValue("storage.docid_drum.bucket_byte_size", docid_drum_bucket_byte_size_))
    {
        docid_drum_bucket_byte_size_ = DEFAULT_DRUM_BUCKET_BYTE_SIZE;
    }
    else
    {
        docid_drum_bucket_byte_size_ = std::min(MAX_DRUM_BUCKET_BYTE_SIZE, docid_drum_bucket_byte_size_);
    }
}


void LogServerCfg::parseDriverCollections(const std::string& collections)
{
    boost::char_separator<char> sep(", ");
    boost::tokenizer<char_separator<char> > tokens(collections, sep);

    boost::tokenizer<char_separator<char> >::iterator it;
    for(it = tokens.begin(); it != tokens.end(); ++it)
    {
        driverCollections_.insert(*it);
    }
}

}

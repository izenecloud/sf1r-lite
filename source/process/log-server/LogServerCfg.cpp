#include "LogServerCfg.h"

#include <util/kv2string.h>
#include <util/string/StringUtils.h>

#include <iostream>
#include <fstream>

#include <boost/tokenizer.hpp>

namespace sf1r
{

typedef izenelib::util::kv2string properties;

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

        if (!props.getValue("logServer.host", host_))
        {
            host_ = "localhost";
        }

        if (!props.getValue("logServer.rpcPort", rpcPort_))
        {
            throw std::runtime_error("Log Server Configuration missing proptery: logServer.rpcPort");
        }

        if (!props.getValue("logServer.driverPort", driverPort_))
        {
            throw std::runtime_error("Log Server Configuration missing proptery: logServer.driverPort");
        }

        if (!props.getValue("logServer.threadNum", threadNum_))
        {
            threadNum_ = DEFAULT_THREAD_NUM;
        }
        else
        {
            threadNum_ = std::min(MAX_THREAD_NUM, threadNum_);
        }

        std::string collections;
        if (props.getValue("driver.collections", collections))
        {
            parseDriverCollections(collections);
        }

        if (!props.getValue("drum.name", drum_name_))
        {
            throw std::runtime_error("Log Server Configuration missing proptery: drum.name");
        }

        if (!props.getValue("drum.num_buckets", drum_num_buckets_))
        {
            drum_num_buckets_ = DEFAULT_DRUM_NUM_BUCKETS;
        }
        else
        {
            drum_num_buckets_ = std::min(MAX_DRUM_NUM_BUCKETS, drum_num_buckets_);
        }

        if (!props.getValue("drum.bucket_buff_elem_size", drum_bucket_buff_elem_size_))
        {
            drum_bucket_buff_elem_size_ = DEFAULT_DRUM_BUCKET_BUFF_ELEM_SIZE;
        }
        else
        {
            drum_bucket_buff_elem_size_ = std::min(MAX_DRUM_BUCKET_BUFF_ELEM_SIZE, drum_bucket_buff_elem_size_);
        }

        if (!props.getValue("drum.bucket_byte_size", drum_bucket_byte_size_))
        {
            drum_bucket_byte_size_ = DEFAULT_DRUM_BUCKET_BYTE_SIZE;
        }
        else
        {
            drum_bucket_byte_size_ = std::min(MAX_DRUM_BUCKET_BYTE_SIZE, drum_bucket_byte_size_);
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return false;
    }

    return true;
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

#include "LogServerCfg.h"

#include <util/kv2string.h>
#include <util/string/StringUtils.h>

#include <iostream>
#include <fstream>

namespace sf1r
{

typedef izenelib::util::kv2string properties;

static const unsigned int DEFAULT_THREAD_NUM = 30;


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
            //throw std::runtime_error("Log Server Configuration missing proptery: logServer.host");
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
            //throw std::runtime_error("Log Server Configuration missing proptery: logServer.threadNum");
            threadNum_ = DEFAULT_THREAD_NUM;
        }
        if (!props.getValue("drum.name", drum_name_))
        {
            throw std::runtime_error("Log Server Configuration missing proptery: drum.name");
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return false;
    }

    return true;
}

}

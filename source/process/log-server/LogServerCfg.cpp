#include "LogServerCfg.h"

#include <util/kv2string.h>
#include <util/string/StringUtils.h>

#include <iostream>
#include <fstream>


using namespace sf1r;

typedef izenelib::util::kv2string properties;

LogServerCfg::LogServerCfg()
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
            std::cerr<<"Could not open file: "<<cfgFile<<std::endl;
            return false;
        }

        while(getline(cfgInput, line))
        {
            //std::cout<<line<<std::endl;
            // ignore empty line and comment line
            izenelib::util::Trim(line);
            if (line.empty() || line.at(0) == '#')
            {
                continue;
            }

            if (!cfgString.empty())
            {
                cfgString += '\n';
            }
            cfgString += line;
            //std::cout<<"->"<<cfgString<<"<-"<<std::endl;
        }

        // check configuration properties
        properties props('\n', '=');
        props.loadKvString(cfgString);

        if (!props.getValue("logServer.host", host_))
        {
            throw std::runtime_error("Log Server Configuration missing proptery: logServer.host");
        }
        if (!props.getValue("logServer.port", port_))
        {
            throw std::runtime_error("Log Server Configuration missing proptery: logServer.port");
        }
    }
    catch (std::exception& e)
    {
        std::cerr<<e.what()<<std::endl;
        return false;
    }

    return true;
}

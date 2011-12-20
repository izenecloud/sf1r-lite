#include "LogServer.h"

#include <iostream>

namespace sf1r
{

LogServer::LogServer()
{
}

bool LogServer::init(const std::string& cfgFile)
{
    return logServerCfg_.parse(cfgFile);
}

void LogServer::start()
{
    std::cout <<"Start LogServer "
              <<logServerCfg_.getLocalHost()<<":"<<logServerCfg_.getServerPort()<<std::endl;
}

void LogServer::stop()
{

}

}


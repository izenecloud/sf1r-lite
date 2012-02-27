#include "RemoteItemIdGeneratorTestFixture.h"
#include <log-manager/LogServerConnection.h>
#include <log-server/LogServerCfg.h>

#include <boost/filesystem.hpp>
#include <iostream>
#include <fstream>

namespace
{
const std::string LOG_SERVER_CFG_FILE = "logserver.cfg";
const std::string HOST = "localhost";
const int RPC_PORT = 19002;
const int DRIVER_PORT = 19003;

bool createLogServerConfigFile(const std::string& baseDir, const std::string& filePath)
{
    using std::endl;

    std::ofstream ofs(filePath.c_str());

    if (! ofs)
    {
        std::cerr << "failed to create " << filePath << endl;
        return false;
    }

    ofs << "host=" << HOST << endl;
    ofs << "rpc.port=" << RPC_PORT << endl;
    ofs << "storage.base_dir=" << baseDir << endl << endl;

    ofs << "driver.port=" << DRIVER_PORT << endl
        << "storage.uuid_drum.name=uuid_drum" << endl
        << "storage.docid_drum.name=docid_drum" << endl;

    return true;
}

}

namespace sf1r
{

RemoteItemIdGeneratorTestFixture::RemoteItemIdGeneratorTestFixture()
    : connectionConfig_(HOST, RPC_PORT)
    , connection_(LogServerConnection::instance())
{
    connection_.init(connectionConfig_);
}

bool RemoteItemIdGeneratorTestFixture::startRpcLogServer(const std::string& baseDir)
{
    std::string configFile = (boost::filesystem::path(baseDir) / LOG_SERVER_CFG_FILE).string();
    LogServerCfg* logServerCfg = LogServerCfg::get();

    if (!createLogServerConfigFile(baseDir, configFile) ||
        !logServerCfg->parse(configFile))
    {
        return false;
    }

    const std::string& localhost = logServerCfg->getLocalHost();
    unsigned int rpcServerPort = logServerCfg->getRpcServerPort();
    unsigned int rpcThreadNum = logServerCfg->getRpcThreadNum();

    rpcLogServer_.reset(new RpcLogServer(localhost, rpcServerPort, rpcThreadNum));
    if (rpcLogServer_->init())
    {
        rpcLogServer_->start();
        return true;
    }

    return false;
}

} // namespace sf1r

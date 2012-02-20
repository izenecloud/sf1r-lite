#include "ItemIdGeneratorTestFixture.h"
#include <recommend-manager/item/ItemIdGenerator.h>
#include <log-manager/LogServerConnection.h>
#include <log-server/LogServerCfg.h>
#include <util/test/BoostTestThreadSafety.h>

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
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

ItemIdGeneratorTestFixture::ItemIdGeneratorTestFixture()
    : maxItemId_(0)
    , connectionConfig_(HOST, RPC_PORT)
    , connection_(LogServerConnection::instance())
{
    connection_.init(connectionConfig_);
}

void ItemIdGeneratorTestFixture::checkNonExistId()
{
    BOOST_CHECK_EQUAL(itemIdGenerator_->maxItemId(), maxItemId_);

    std::string strId;
    BOOST_CHECK(itemIdGenerator_->itemIdToStrId(maxItemId_+1, strId) == false);
}

void ItemIdGeneratorTestFixture::checkStrToItemId(itemid_t maxItemId)
{
    for (itemid_t goldId=1; goldId<=maxItemId; ++goldId)
    {
        std::string str = boost::lexical_cast<std::string>(goldId);
        itemid_t id = 0;

        BOOST_CHECK_TS(itemIdGenerator_->strIdToItemId(str, id));
        BOOST_CHECK_EQUAL_TS(id, goldId);
    }

    BOOST_CHECK_EQUAL_TS(itemIdGenerator_->maxItemId(), maxItemId);

    maxItemId_ = maxItemId;
}

void ItemIdGeneratorTestFixture::checkItemIdToStr(itemid_t maxItemId)
{
    for (itemid_t id=1; id<=maxItemId; ++id)
    {
        std::string goldStr = boost::lexical_cast<std::string>(id);
        std::string str;

        BOOST_CHECK_TS(itemIdGenerator_->itemIdToStrId(id, str));
        BOOST_CHECK_EQUAL_TS(str, goldStr);
    }

    BOOST_CHECK_EQUAL_TS(itemIdGenerator_->maxItemId(), maxItemId);
}

void ItemIdGeneratorTestFixture::checkMultiThread(int threadNum, itemid_t maxItemId)
{
    boost::thread_group threads;

    for (int i=0; i<threadNum; ++i)
    {
        threads.create_thread(boost::bind(&ItemIdGeneratorTestFixture::checkStrToItemId,
                                          this, maxItemId));
    }

    threads.join_all();

    checkStrToItemId(maxItemId);
    checkItemIdToStr(maxItemId);
}

bool ItemIdGeneratorTestFixture::startRpcLogServer(const std::string& baseDir)
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

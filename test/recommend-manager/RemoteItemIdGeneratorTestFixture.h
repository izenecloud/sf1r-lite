///
/// @file RemoteItemIdGeneratorTestFixture.h
/// @brief fixture class to test RemoteItemIdGenerator
/// @author Jun Jiang
/// @date Created 2012-02-27
///

#ifndef REMOTE_ITEM_ID_GENERATOR_TEST_FIXTURE
#define REMOTE_ITEM_ID_GENERATOR_TEST_FIXTURE

#include "ItemIdGeneratorTestFixture.h"
#include <configuration-manager/LogServerConnectionConfig.h>
#include <log-server/RpcLogServer.h>

#include <string>
#include <boost/scoped_ptr.hpp>

namespace sf1r
{
class LogServerConnection;

class RemoteItemIdGeneratorTestFixture : public ItemIdGeneratorTestFixture
{
public:
    RemoteItemIdGeneratorTestFixture();

    bool startRpcLogServer(const std::string& baseDir);

protected:
    LogServerConnectionConfig connectionConfig_;
    LogServerConnection& connection_;
    boost::scoped_ptr<RpcLogServer> rpcLogServer_;
};

} // namespace sf1r

#endif //REMOTE_ITEM_ID_GENERATOR_TEST_FIXTURE

///
/// @file ItemIdGeneratorTestFixture.h
/// @brief fixture class to test ItemIdGenerator implementations
/// @author Jun Jiang
/// @date Created 2012-02-16
///

#ifndef ITEM_ID_GENERATOR_TEST_FIXTURE
#define ITEM_ID_GENERATOR_TEST_FIXTURE

#include <recommend-manager/common/RecTypes.h>
#include <configuration-manager/LogServerConnectionConfig.h>
#include <log-server/RpcLogServer.h>

#include <string>
#include <boost/scoped_ptr.hpp>

namespace sf1r
{
class ItemIdGenerator;
class LogServerConnection;

class ItemIdGeneratorTestFixture
{
public:
    ItemIdGeneratorTestFixture();

    /**
     * it should fail to convert from a non-exist id to string.
     */
    void checkNonExistId();

    /**
     * Iterate id from 1 to @p maxItemId,
     * it should convert each id from string to numeric value.
     */
    void checkStrToItemId(itemid_t maxItemId);

    /**
     * This function should be called after @c checkStrToItemId(maxItemId),
     * it iterates id from 1 to @p maxItemId,
     * it should convert each id from numeric value back to string.
     */
    void checkItemIdToStr(itemid_t maxItemId);

    /**
     * Create @p threadNum threads, each thread checks id from 1 to @p maxItemId.
     */
    void checkMultiThread(int threadNum, itemid_t maxItemId);

    bool startRpcLogServer(const std::string& baseDir);

protected:
    itemid_t maxItemId_;
    boost::scoped_ptr<ItemIdGenerator> itemIdGenerator_;

    LogServerConnectionConfig connectionConfig_;
    LogServerConnection& connection_;
    boost::scoped_ptr<RpcLogServer> rpcLogServer_;
};

} // namespace sf1r

#endif //ITEM_ID_GENERATOR_TEST_FIXTURE

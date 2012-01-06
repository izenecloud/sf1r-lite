#include <log-manager/LogServerRequest.h>
#include <log-manager/LogServerConnection.h>

#include <util/ClockTimer.h>
#include <common/Utilities.h>

#include <boost/uuid/random_generator.hpp>

using namespace sf1r;

void t_RpcLogServer();
void t_RpcLogServerCreateTestData();
void t_RpcLogServerUpdateTestData();

int main(int argc, char* argv[])
{
    try
    {
        if (argc > 1 && strcmp(argv[1], "-ct") == 0)
        {
            t_RpcLogServerCreateTestData();
        }
        else if (argc > 1 && strcmp(argv[1], "-ut") == 0)
        {
            t_RpcLogServerUpdateTestData();
        }
        else
        {
            t_RpcLogServer();
        }

    }
    catch(std::exception& e)
    {
        std::cout<<e.what()<<std::endl;
    }

    return 0;
}

void t_RpcLogServer()
{
    LogServerConnection& conn = LogServerConnection::instance();
    conn.init("localhost", 18811);

    UpdateUUIDRequest uuidReq;
    uuidReq.param_.docidList_.push_back(1111);
    uuidReq.param_.docidList_.push_back(2222);
    uuidReq.param_.docidList_.push_back(3333);

    izenelib::util::ClockTimer t;
    boost::uuids::random_generator random_gen;
    for (int i = 0; i < 1; i++)
    {
        for (int j = 0; j < 0x1000000; j++)
        {
            boost::uuids::uuid uuid = random_gen();
            uuidReq.param_.uuid_ = *reinterpret_cast<uint128_t *>(&uuid);
            conn.asynRequest(uuidReq);
        }
    }

    // Force server to synchronize
    SynchronizeRequest syncReq;
    conn.asynRequest(syncReq);

    conn.flushRequests();
    std::cout << "time elapsed for inserting " << t.elapsed() << std::endl;
}



void t_RpcLogServerCreateTestData()
{
    LogServerConnection& conn = LogServerConnection::instance();
    conn.init("localhost", 18811);

    UpdateUUIDRequest uuidReq;

    uuidReq.param_.docidList_.push_back(11110);
    uuidReq.param_.docidList_.push_back(22220);
    uuidReq.param_.docidList_.push_back(33330);
    uuidReq.param_.uuid_ = Utilities::uuidToUint128("143c7d31-702e-4fac-b57b-84d35205ae60");
    conn.asynRequest(uuidReq);
    std::cout << "sent rpc request: " << uuidReq.param_.toString() << std::endl;

    uuidReq.param_.docidList_.clear();
    uuidReq.param_.docidList_.push_back(44440);
    uuidReq.param_.docidList_.push_back(55550);
    uuidReq.param_.uuid_ = Utilities::uuidToUint128("cda5545a-b3f4-4e81-9b85-2d25b0416997");
    conn.asynRequest(uuidReq);
    std::cout << "sent rpc request: " << uuidReq.param_.toString() << std::endl;

    uuidReq.param_.docidList_.clear();
    uuidReq.param_.docidList_.push_back(66660);
    uuidReq.param_.docidList_.push_back(77770);
    uuidReq.param_.uuid_ = Utilities::uuidToUint128("eb1ba5f4-a558-4a66-806d-74cb6a321932");
    conn.asynRequest(uuidReq);
    std::cout << "sent rpc request: " << uuidReq.param_.toString() << std::endl;

    uuidReq.param_.docidList_.clear();
    uuidReq.param_.docidList_.push_back(88880);
    uuidReq.param_.docidList_.push_back(99990);
    uuidReq.param_.uuid_ = Utilities::uuidToUint128("dcdce290-b73d-467b-aa44-755cce035c79");
    conn.asynRequest(uuidReq);
    std::cout << "sent rpc request: " << uuidReq.param_.toString() << std::endl;

    conn.flushRequests();

    // =====> cclog after uuid generated

    // {"collection":"b5ma","resource":{"session_id":"","USERID":"","ITEMID":"143c7d31-702e-4fac-b57b-84d35205ae60","is_recommend_item":"false"},"header":{"controller":"recommend","action":"visit_item"}}

    // {"resource":{"DOCID":"cda5545a-b3f4-4e81-9b85-2d25b0416997"},"collection":"b5ma","header":{"controller":"documents","action":"visit"}}

    // {"collection":"b5ma","resource":{"USERID":"","items":[{"ITEMID":"eb1ba5f4-a558-4a66-806d-74cb6a321932","price":1088,"quantity":1},{"ITEMID":"dcdce290-b73d-467b-aa44-755cce035c79","price":65,"quantity":1}]},"header":{"controller":"recommend","action":"purchase_item"}}
}

void t_RpcLogServerUpdateTestData()
{
    LogServerConnection& conn = LogServerConnection::instance();
    conn.init("localhost", 18811);

    UpdateUUIDRequest uuidReq;

    uuidReq.param_.docidList_.push_back(11110);
    uuidReq.param_.docidList_.push_back(22220);
    uuidReq.param_.uuid_ = Utilities::uuidToUint128("cc1e948d-ebf2-4a25-bf58-cb0c25ee65c1");
    conn.asynRequest(uuidReq);
    std::cout << "sent rpc request: " << uuidReq.param_.toString() << std::endl;

    uuidReq.param_.docidList_.clear();
    uuidReq.param_.docidList_.push_back(33330); //
    uuidReq.param_.docidList_.push_back(44440);
    uuidReq.param_.docidList_.push_back(55550);
    uuidReq.param_.uuid_ = Utilities::uuidToUint128("366f5977-aa73-412f-83f3-66fbe06a6b40");
    conn.asynRequest(uuidReq);
    std::cout << "sent rpc request: " << uuidReq.param_.toString() << std::endl;

    uuidReq.param_.docidList_.clear();
    uuidReq.param_.docidList_.push_back(66660);
    uuidReq.param_.docidList_.push_back(77770);
    uuidReq.param_.uuid_ = Utilities::uuidToUint128("6d5f82db-0ef2-4d5f-8b54-cdc29e97e5b1");
    conn.asynRequest(uuidReq);
    std::cout << "sent rpc request: " << uuidReq.param_.toString() << std::endl;

    uuidReq.param_.docidList_.clear();
    uuidReq.param_.docidList_.push_back(88880);
    uuidReq.param_.docidList_.push_back(99990);
    uuidReq.param_.uuid_ = Utilities::uuidToUint128("4857bcda-22e4-4a87-964d-17ffcf024d16");
    conn.asynRequest(uuidReq);
    std::cout << "sent rpc request: " << uuidReq.param_.toString() << std::endl;

    conn.flushRequests();

    // =====> update cclog (by driver log server) after uuid updated

    // {"collection":"b5ma","resource":{"session_id":"","USERID":"","ITEMID":"cc1e948d-ebf2-4a25-bf58-cb0c25ee65c1","is_recommend_item":"false"},"header":{"controller":"recommend","action":"visit_item"}}
    // {"collection":"b5ma","resource":{"session_id":"","USERID":"","ITEMID":"366f5977-aa73-412f-83f3-66fbe06a6b40","is_recommend_item":"false"},"header":{"controller":"recommend","action":"visit_item"}}

    // {"resource":{"DOCID":"366f5977-aa73-412f-83f3-66fbe06a6b40"},"collection":"b5ma","header":{"controller":"documents","action":"visit"}}

    // {"collection":"b5ma","resource":{"USERID":"","items":[{"ITEMID":"6d5f82db-0ef2-4d5f-8b54-cdc29e97e5b1","price":1088,"quantity":1},{"ITEMID":"4857bcda-22e4-4a87-964d-17ffcf024d16","price":65,"quantity":1}]},"header":{"controller":"recommend","action":"purchase_item"}}
}




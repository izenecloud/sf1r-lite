#include <log-manager/LogServerRequest.h>
#include <log-manager/LogServerConnection.h>

#include <util/ClockTimer.h>
#include <common/Utilities.h>

#include <boost/uuid/random_generator.hpp>

using namespace sf1r;

void t_RpcLogServer();
void t_RpcLogServerTestData();

int main(int argc, char* argv[])
{
    try
    {
        t_RpcLogServerTestData();
        //t_RpcLogServer();
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

//    UUID2DocIdList uuidReqData;
//    uuidReqData.uuid_ = "123456789abcdef0";
//    conn.asynRequest(LogServerRequest::METHOD_UPDATE_UUID, uuidReqData);
//
//    UpdateUUIDRequest uuidReq;
//    uuidReq.param_.uuid_ = "123456789abcdef1";
//    conn.asynRequest(uuidReq);

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
    conn.flushRequests();
    std::cout << "time elapsed for inserting " << t.elapsed() << std::endl;
}

void t_RpcLogServerTestData()
{
    LogServerConnection& conn = LogServerConnection::instance();
    conn.init("localhost", 18811);

    UpdateUUIDRequest uuidReq;

    uuidReq.param_.docidList_.push_back(1111);
    uuidReq.param_.docidList_.push_back(2222);
    uuidReq.param_.docidList_.push_back(3333);
    uuidReq.param_.uuid_ = Utilities::hexStringToUint128("143c7d31702e4facb57b84d35205ae60");
    conn.asynRequest(uuidReq);

    uuidReq.param_.docidList_.push_back(4444);
    uuidReq.param_.docidList_.push_back(5555);
    uuidReq.param_.uuid_ = Utilities::hexStringToUint128("cda5545ab3f44e819b852d25b0416997");
    conn.asynRequest(uuidReq);

    conn.flushRequests();
}


//{"collection":"b5ma","resource":{"session_id":"","USERID":"","ITEMID":"143c7d31702e4facb57b84d35205ae60","is_recommend_item":"false"},"header":{"controller":"recommend","action":"visit_item"}}
//{"collection":"b5ma","resource":{"session_id":"","USERID":"","ITEMID":"e2ecc3000e2742049eba2a73ebda8013","is_recommend_item":"false"},"header":{"controller":"recommend","action":"visit_item"}}
//{"resource":{"DOCID":"cda5545ab3f44e819b852d25b0416997"},"collection":"b5ma","header":{"controller":"documents","action":"visit"}}
//{"resource":{"DOCID":"027b6fed167c46fea83c73c6f6e209a2"},"collection":"b5ma","header":{"controller":"documents","action":"visit"}}
//{"collection":"b5ma","resource":{"USERID":"","items":[{"ITEMID":"6bf634c51b4f4772bb52224b94c17af9","price":19,"quantity":1}]},"header":{"controller":"recommend","action":"purchase_item"}}
//{"collection":"b5ma","resource":{"USERID":"","items":[{"ITEMID":"623999b623b24be5b12531fda9e1439f","price":11880,"quantity":1}]},"header":{"controller":"recommend","action":"purchase_item"}}
//{"collection":"b5ma","resource":{"USERID":"","items":[{"ITEMID":"eb1ba5f4a5584a66806d74cb6a321932","price":1088,"quantity":1},{"ITEMID":"dcdce290b73d467baa44755cce035c79","price":65,"quantity":1}]},"header":{"controller":"recommend","action":"purchase_item"}}







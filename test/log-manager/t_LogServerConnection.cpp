#include <log-manager/LogServerRequest.h>
#include <log-manager/LogServerConnection.h>

#include <util/ClockTimer.h>
#include <common/Utilities.h>

#include <boost/uuid/random_generator.hpp>

using namespace sf1r;

void t_RpcLogServer();
void t_RpcLogServerCreateTestData();
void t_RpcLogServerUpdateTestData();
void t_ScdStorage();

int main(int argc, char* argv[])
{
    try
    {
        if (argc > 1 && strcmp(argv[1], "--build_info") == 0)
        {
            // avoid auto testing by Jenkins
            return 0;
        }

        if (argc > 1 && strcmp(argv[1], "-ct") == 0)
        {
            t_RpcLogServerCreateTestData();
        }
        else if (argc > 1 && strcmp(argv[1], "-ut") == 0)
        {
            t_RpcLogServerUpdateTestData();
        }
        else if (argc > 1 && strcmp(argv[1], "-s") == 0)
        {
            t_ScdStorage();
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
    uuidReq.param_.docidList_.resize(3);

    izenelib::util::ClockTimer t;
    boost::uuids::random_generator random_gen;
    std::size_t REQUESTS_NUM = 0x1000000;

    std::cout << "sending requests:" << std::endl;
    for (std::size_t  i = 0; i < REQUESTS_NUM; i++)
    {
        boost::uuids::uuid uuid = random_gen();
        uuidReq.param_.uuid_ = *reinterpret_cast<uint128_t *>(&uuid);
        uuid = random_gen();
        uuidReq.param_.docidList_[0] = *reinterpret_cast<uint128_t *>(&uuid);
        uuid = random_gen();
        uuidReq.param_.docidList_[1] = *reinterpret_cast<uint128_t *>(&uuid);
        uuid = random_gen();
        uuidReq.param_.docidList_[2] = *reinterpret_cast<uint128_t *>(&uuid);
        conn.asynRequest(uuidReq);

        if (i%10000 == 0)
        {
            std::cout<<"\r"<<std::left<<setw(16)<<i<<"\t"<<int(i*100.0/REQUESTS_NUM)<<"%"<<std::flush;
        }
    }
    std::cout<<"\r"<<std::left<<setw(16)<<REQUESTS_NUM<<"\t100%"<<std::endl;

    std::cout << "flushing requests" << std::endl;
    conn.flushRequests();

    std::cout << "time elapsed for inserting " << t.elapsed() << std::endl;

    // Force server to synchronize
    std::cout << "forcing server to synchronize" << std::endl;
    SynchronizeRequest syncReq;
    conn.asynRequest(syncReq);
}

void t_RpcLogServerCreateTestData()
{
    LogServerConnection& conn = LogServerConnection::instance();
    conn.init("localhost", 18811);

    UpdateUUIDRequest uuidReq;

    uuidReq.param_.docidList_.push_back(Utilities::md5ToUint128("04e0f81ab8119642b93bbf339369a4a1"));
    uuidReq.param_.docidList_.push_back(Utilities::md5ToUint128("04e0f81ab8119642b93bbf339369a4a2"));
    uuidReq.param_.docidList_.push_back(Utilities::md5ToUint128("04e0f81ab8119642b93bbf339369a4a3"));
    uuidReq.param_.uuid_ = Utilities::uuidToUint128("143c7d31-702e-4fac-b57b-84d35205ae60");
    conn.asynRequest(uuidReq);
    std::cout << "sent rpc request: " << uuidReq.param_.toString() << std::endl;

    uuidReq.param_.docidList_.clear();
    uuidReq.param_.docidList_.push_back(Utilities::md5ToUint128("04e0f81ab8119642b93bbf339369a4a4"));
    uuidReq.param_.docidList_.push_back(Utilities::md5ToUint128("04e0f81ab8119642b93bbf339369a4a5"));
    uuidReq.param_.uuid_ = Utilities::uuidToUint128("cda5545a-b3f4-4e81-9b85-2d25b0416997");
    conn.asynRequest(uuidReq);
    std::cout << "sent rpc request: " << uuidReq.param_.toString() << std::endl;

    uuidReq.param_.docidList_.clear();
    uuidReq.param_.docidList_.push_back(Utilities::md5ToUint128("04e0f81ab8119642b93bbf339369a4a6"));
    uuidReq.param_.docidList_.push_back(Utilities::md5ToUint128("04e0f81ab8119642b93bbf339369a4a7"));
    uuidReq.param_.uuid_ = Utilities::uuidToUint128("eb1ba5f4-a558-4a66-806d-74cb6a321932");
    conn.asynRequest(uuidReq);
    std::cout << "sent rpc request: " << uuidReq.param_.toString() << std::endl;

    uuidReq.param_.docidList_.clear();
    uuidReq.param_.docidList_.push_back(Utilities::md5ToUint128("04e0f81ab8119642b93bbf339369a4a8"));
    uuidReq.param_.docidList_.push_back(Utilities::md5ToUint128("04e0f81ab8119642b93bbf339369a4a9"));
    uuidReq.param_.uuid_ = Utilities::uuidToUint128("dcdce290-b73d-467b-aa44-755cce035c79");
    conn.asynRequest(uuidReq);
    std::cout << "sent rpc request: " << uuidReq.param_.toString() << std::endl;

    SynchronizeRequest syncReq;
    conn.asynRequest(syncReq);

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

    uuidReq.param_.docidList_.push_back(Utilities::md5ToUint128("04e0f81ab8119642b93bbf339369a4a1"));
    uuidReq.param_.docidList_.push_back(Utilities::md5ToUint128("04e0f81ab8119642b93bbf339369a4a2"));
    uuidReq.param_.uuid_ = Utilities::uuidToUint128("cc1e948d-ebf2-4a25-bf58-cb0c25ee65c1");
    conn.asynRequest(uuidReq);
    std::cout << "sent rpc request: " << uuidReq.param_.toString() << std::endl;

    uuidReq.param_.docidList_.clear();
    uuidReq.param_.docidList_.push_back(Utilities::md5ToUint128("04e0f81ab8119642b93bbf339369a4a3")); //
    uuidReq.param_.docidList_.push_back(Utilities::md5ToUint128("04e0f81ab8119642b93bbf339369a4a4"));
    uuidReq.param_.docidList_.push_back(Utilities::md5ToUint128("04e0f81ab8119642b93bbf339369a4a5"));
    uuidReq.param_.uuid_ = Utilities::uuidToUint128("366f5977-aa73-412f-83f3-66fbe06a6b40");
    conn.asynRequest(uuidReq);
    std::cout << "sent rpc request: " << uuidReq.param_.toString() << std::endl;

    uuidReq.param_.docidList_.clear();
    uuidReq.param_.docidList_.push_back(Utilities::md5ToUint128("04e0f81ab8119642b93bbf339369a4a6"));
    uuidReq.param_.docidList_.push_back(Utilities::md5ToUint128("04e0f81ab8119642b93bbf339369a4a7"));
    uuidReq.param_.uuid_ = Utilities::uuidToUint128("6d5f82db-0ef2-4d5f-8b54-cdc29e97e5b1");
    conn.asynRequest(uuidReq);
    std::cout << "sent rpc request: " << uuidReq.param_.toString() << std::endl;

    uuidReq.param_.docidList_.clear();
    uuidReq.param_.docidList_.push_back(Utilities::md5ToUint128("04e0f81ab8119642b93bbf339369a4a8"));
    uuidReq.param_.docidList_.push_back(Utilities::md5ToUint128("04e0f81ab8119642b93bbf339369a4a9"));
    uuidReq.param_.uuid_ = Utilities::uuidToUint128("4857bcda-22e4-4a87-964d-17ffcf024d16");
    conn.asynRequest(uuidReq);
    std::cout << "sent rpc request: " << uuidReq.param_.toString() << std::endl;

    SynchronizeRequest syncReq;
    conn.asynRequest(syncReq);

    conn.flushRequests();

    // =====> update cclog (by driver log server) after uuid updated

    // {"collection":"b5ma","resource":{"session_id":"","USERID":"","ITEMID":"cc1e948d-ebf2-4a25-bf58-cb0c25ee65c1","is_recommend_item":"false"},"header":{"controller":"recommend","action":"visit_item"}}
    // {"collection":"b5ma","resource":{"session_id":"","USERID":"","ITEMID":"366f5977-aa73-412f-83f3-66fbe06a6b40","is_recommend_item":"false"},"header":{"controller":"recommend","action":"visit_item"}}

    // {"resource":{"DOCID":"366f5977-aa73-412f-83f3-66fbe06a6b40"},"collection":"b5ma","header":{"controller":"documents","action":"visit"}}

    // {"collection":"b5ma","resource":{"USERID":"","items":[{"ITEMID":"6d5f82db-0ef2-4d5f-8b54-cdc29e97e5b1","price":1088,"quantity":1},{"ITEMID":"4857bcda-22e4-4a87-964d-17ffcf024d16","price":65,"quantity":1}]},"header":{"controller":"recommend","action":"purchase_item"}}
}

void t_ScdStorage()
{
    LogServerConnection& conn = LogServerConnection::instance();
    conn.init("localhost", 18811);

    // create scd doc
    CreateScdDocRequest scdDocReq;
    scdDocReq.param_.uuid_ = Utilities::uuidToUint128("143c7d31-702e-4fac-b57b-84d35205ae60");
    scdDocReq.param_.content_ = //"This is doc1!!";
            "<DOCID>143c7d31-702e-4fac-b57b-84d35205ae60\n"
            "<ProdName>儿童创意美工.巧手贴画\n";
    conn.asynRequest(scdDocReq);
    scdDocReq.param_.uuid_ = Utilities::uuidToUint128("cda5545a-b3f4-4e81-9b85-2d25b0416997");
    scdDocReq.param_.content_ = //"This is doc2!!";
            "<DOCID>cda5545a-b3f4-4e81-9b85-2d25b0416997\n"
            "<ProdName>全脑智能数学A5――幼儿智能全面开发应用操作课本\n";
    conn.asynRequest(scdDocReq);
    scdDocReq.param_.uuid_ = Utilities::uuidToUint128("eb1ba5f4-a558-4a66-806d-74cb6a321932");
    scdDocReq.param_.content_ = //"This is doc3!!";
            "<DOCID>eb1ba5f4-a558-4a66-806d-74cb6a321932\n"
            "<ProdName>百诺 旅游天使 A2681TB1 镁合金 三脚架\n";
    conn.asynRequest(scdDocReq);
    conn.flushRequests();

    // get scd file
    GetScdFileRequest scdFileReq;
    scdFileReq.param_.username_ = "zhongxia";
    scdFileReq.param_.host_ = "localhost";
    scdFileReq.param_.path_ = "/home/zhongxia/scd/";

    GetScdFileResponseData ret;
    conn.syncRequest(scdFileReq, ret);
    if (ret.success_)
    {
        std::cout << "Successfully Fetched SCD." << std::endl;
    }
    else
    {
        std::cout << "Failed to fetch SCD." << std::endl;
        std::cout << ret.error_ << std::endl;
    }
}



#include <log-manager/LogServerRequest.h>
#include <log-manager/LogServerConnection.h>

#include <util/ClockTimer.h>

#include <boost/uuid/random_generator.hpp>

using namespace sf1r;

void t_RpcLogServer();

int main(int argc, char* argv[])
{
    try
    {
        t_RpcLogServer();
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

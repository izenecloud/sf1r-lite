#include <log-manager/LogServerRequest.h>
#include <log-manager/LogServerConnection.h>

#include <sstream>

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

    for (int i = 0; i < 16; i++)
    {
        std::stringstream ss;
        ss << "123456789abcdef" << hex << i;
        uuidReq.param_.uuid_ = ss.str();

        conn.asynRequest(uuidReq);
    }
}

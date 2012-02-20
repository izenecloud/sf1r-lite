/**
 * @file MasterServerConnector.h
 * @author Zhongxia Li
 * @date Feb 14, 2012
 * @brief Connector for Master Server
 */
#ifndef MASTER_SERVER_CONNECTOR_H_
#define MASTER_SERVER_CONNECTOR_H_

#include <util/singleton.h>

#include <3rdparty/msgpack/rpc/client.h>
#include <3rdparty/msgpack/rpc/request.h>

#include <node-manager/RecommendMasterManager.h>

namespace sf1r
{


class MasterServerConnector
{
public:
    enum MethodId
    {
        Method_getDocumentsByIds_,
        Method_Number
    };

    static const std::string Methods_[Method_Number];

public:
    MasterServerConnector()
    {
        // xxx or recommend master server
        findSearchMaster();
    }

    static MasterServerConnector* get()
    {
        return izenelib::util::Singleton<MasterServerConnector>::get();
    }

    template <class RequestDataT>
    void asyncCall(const MethodId& methodId, const RequestDataT& reqData)
    {
        if (!isFoundMaster_)
        {
            findSearchMaster();
        }

        try
        {
            msgpack::rpc::client client(host_, port_);
            client.call(Methods_[methodId], reqData);
            client.get_loop()->flush();
        }
        catch (const std::exception& e)
        {
            std::cerr << e.what();
        }
    }

    template <class RequestDataT, class ResponseDataT>
    void syncCall(const MethodId& methodId, const RequestDataT& reqData, ResponseDataT& respData)
    {
        if (!isFoundMaster_)
        {
            findSearchMaster();
        }

        try
        {
            msgpack::rpc::client client(host_, port_);
            msgpack::rpc::future f = client.call(Methods_[methodId], reqData);
            respData = f.get<ResponseDataT>();
            client.get_loop()->flush();
        }
        catch (const std::exception& e)
        {
            std::cerr << e.what();
        }
    }

private:
    void findSearchMaster()
    {
        if (RecommendMasterManager::get()->getSearchMasterAddress(host_, port_))
        {
            std::cout << "Found search master: " << host_ << " : " << port_ << std::endl;
            isFoundMaster_ = true;
        }
        else
        {
            std::cout << "Failed to detect search master." << std::endl;
            isFoundMaster_ = false;
        }
    }

private:
    bool isFoundMaster_;

    std::string host_;
    uint32_t port_;
};

}

#endif /* MASTER_SERVER_CONNECTOR_H_ */

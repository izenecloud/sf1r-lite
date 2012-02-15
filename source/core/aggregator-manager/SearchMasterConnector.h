/**
 * @file SearchMasterConnector.h
 * @author Zhongxia Li
 * @date Feb 14, 2012
 * @brief 
 */
#ifndef SEARCH_MASTER_CONNECTOR_H_
#define SEARCH_MASTER_CONNECTOR_H_

#include <3rdparty/msgpack/rpc/client.h>
#include <3rdparty/msgpack/rpc/request.h>

#include <node-manager/RecommendMasterManager.h>

namespace sf1r
{

class SearchMasterConnector
{
public:
    SearchMasterConnector()
    {
        findSearchMaster();
    }

    template <typename RequestDataT>
    void asyncCall(const std::string& method, const RequestDataT& reqData)
    {
        if (!isFoundMaster_)
        {
            findSearchMaster();
        }

        try
        {
            msgpack::rpc::client client(host_, port_);
            client.call(method, reqData);
        }
        catch (const std::exception& e)
        {
            ;
        }
    }

    template <typename RequestDataT, typename ResponseDataT>
    ResponseDataT syncCall(const std::string& method, const RequestDataT& reqData)
    {
        if (!isFoundMaster_)
        {
            findSearchMaster();
        }

        try
        {
            msgpack::rpc::client client(host_, port_);
            return client.call(method, reqData).get<ResponseDataT>();
        }
        catch (const std::exception& e)
        {
            ;
        }
    }

private:
    void findSearchMaster()
    {
        if (RecommendMasterManager::get()->getSearchMasterAddress(host_, port_))
        {
            isFoundMaster_ = true;
        }
        else
        {
            isFoundMaster_ = false;
        }
    }

private:
    bool isFoundMaster_;

    std::string host_;
    uint32_t port_;
};

}

#endif /* SEARCH_MASTER_CONNECTOR_H_ */

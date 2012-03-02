/**
 * @file MasterServerConnector.h
 * @author Zhongxia Li
 * @date Feb 14, 2012
 * @brief Connector for Master Server
 */
#ifndef MASTER_SERVER_CONNECTOR_H_
#define MASTER_SERVER_CONNECTOR_H_

#include <node-manager/SearchMasterAddressFinder.h>
#include <util/singleton.h>

#include <3rdparty/msgpack/rpc/session.h>
#include <3rdparty/msgpack/rpc/session_pool.h>

#include <boost/scoped_ptr.hpp>

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
        : isFoundMaster_(false)
        , port_(0)
        , searchMasterAddressFinder_(NULL)
    {
    }

    bool init(SearchMasterAddressFinder* addressFinder, int threadNum = 30)
    {
        searchMasterAddressFinder_ = addressFinder;
        isFoundMaster_ = false;

        try
        {
            session_pool_.reset(new msgpack::rpc::session_pool);
            session_pool_->start(threadNum);
        }
        catch (const std::exception& e)
        {
            std::cerr << e.what() << std::endl;
            return false;
        }

        return true;
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

        msgpack::rpc::session session = session_pool_->get_session(host_, port_);
        session.notify(Methods_[methodId], reqData);
    }

    template <class RequestDataT, class ResponseDataT>
    void syncCall(const MethodId& methodId, const RequestDataT& reqData, ResponseDataT& respData)
    {
        if (!isFoundMaster_)
        {
            findSearchMaster();
        }

        msgpack::rpc::session session = session_pool_->get_session(host_, port_);
        msgpack::rpc::future future = session.call(Methods_[methodId], reqData);
        respData = future.get<ResponseDataT>();
    }

private:
    void findSearchMaster()
    {
        if (searchMasterAddressFinder_ && searchMasterAddressFinder_->findSearchMasterAddress(host_, port_))
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

    SearchMasterAddressFinder* searchMasterAddressFinder_;
    boost::scoped_ptr<msgpack::rpc::session_pool> session_pool_;
};

}

#endif /* MASTER_SERVER_CONNECTOR_H_ */

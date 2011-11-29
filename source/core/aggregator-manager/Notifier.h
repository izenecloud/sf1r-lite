/**
 * @file Notifier.h
 * @author Zhongxia Li
 * @date Sep 1, 2011
 * @brief 
 */
#ifndef AGGREGATOR_NOTIFIER_H_
#define AGGREGATOR_NOTIFIER_H_

#include <3rdparty/msgpack/msgpack.hpp>
#include <3rdparty/msgpack/rpc/client.h>
#include <3rdparty/msgpack/rpc/request.h>
#include <3rdparty/msgpack/rpc/server.h>

#include <util/singleton.h>

namespace sf1r {

struct NotifyMSG
{
    std::string method;
    std::string identity;

    std::string error;

    MSGPACK_DEFINE(method,identity,error);
};

class Notifier
{
public:
    Notifier() :hasMaster_(false) {}

    static Notifier* get()
    {
        return izenelib::util::Singleton<Notifier>::get();
    }

    bool notify(NotifyMSG& msg)
    {
        if (!hasMaster_)
            return true;

        try
        {
            msgpack::rpc::client cli(host_, port_);
            cli.notify(msg.method, msg);
            cli.get_loop()->flush();
        }
        catch (std::exception& e)
        {
            std::cerr<<"**Failed to nofity Master : "<<e.what()<<endl;
            return false;
        }

        return true;
    }

    void setReceiverAddress(const std::string& host, uint16_t port)
    {
        hasMaster_ = true;
        host_ = host;
        port_ = port;
    }

protected:
    bool hasMaster_;
    std::string host_;
    uint16_t port_;
};

}

#endif /* AGGREGATOR_NOTIFIER_H_ */

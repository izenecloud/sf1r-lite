#ifndef SF1_AD_STREAM_SUBSCRIBER_H_
#define SF1_AD_STREAM_SUBSCRIBER_H_

#include <util/singleton.h>
#include <3rdparty/msgpack/rpc/server.h>
#include <string>
#include <sf1r-net/RpcServerRequest.h>
#include <sf1r-net/RpcServerRequestData.h>
#include <sf1r-net/RpcServerConnection.h>
#include <util/concurrent_queue.h>
#include <boost/function.hpp>

namespace sf1r
{

struct AdMessage
{
    std::string topic;
    std::string body;
    MSGPACK_DEFINE(topic, body);
};

struct AdMessageListData
{
    std::vector<AdMessage> msg_list;
    MSGPACK_DEFINE(msg_list);
};

struct SubscribeReqData
{
    std::string topic;
    std::string ip;
    uint16_t port;
    bool un_subscribe;
    MSGPACK_DEFINE(topic, ip, port, un_subscribe);
};

class AdStreamSubscriberServerRequest : public RpcServerRequest
{
public:
    typedef std::string method_t;

    enum METHOD
    {
        METHOD_TEST = 0,
        METHOD_SUBSCRIBE_ADMESSAGE,
        COUNT_OF_METHODS
    };
    static const method_t method_names[COUNT_OF_METHODS];
    AdStreamSubscriberServerRequest(const int& method) : RpcServerRequest(method){}
};

class AdStreamReceiveServerRequest : public RpcServerRequest
{
public:
    typedef std::string method_t;

    enum METHOD
    {
        METHOD_TEST = 0,
        METHOD_PUSH_ADMESSAGE,
        COUNT_OF_METHODS
    };
    static const method_t method_names[COUNT_OF_METHODS];
    AdStreamReceiveServerRequest(const int& method) : RpcServerRequest(method){}
};

class SubscribeAdStreamRequest : public RpcRequestRequestT<SubscribeReqData, AdStreamSubscriberServerRequest>
{
public:
    SubscribeAdStreamRequest()
        :RpcRequestRequestT<SubscribeReqData, AdStreamSubscriberServerRequest>(METHOD_SUBSCRIBE_ADMESSAGE)
    {
    }
};

class AdStreamReceiveServer: public msgpack::rpc::server::base
{
public:
    AdStreamReceiveServer(const std::string& host, uint16_t port, uint32_t threadNum);
    ~AdStreamReceiveServer();
    const std::string& getHost() const
    {
        return host_;
    }
    inline uint16_t getPort() const
    {
        return port_;
    }

    void start();

    void join();

    // start + join
    void run();

    void stop();

public:
    virtual void dispatch(msgpack::rpc::request req);

private:
    std::string host_;
    uint16_t port_;
    uint32_t threadNum_;
};

class AdStreamSubscriber
{
public:
    static AdStreamSubscriber* get()
    {
        return ::izenelib::util::Singleton<AdStreamSubscriber>::get();
    }
    AdStreamSubscriber();
    ~AdStreamSubscriber();
    typedef boost::function<void(const std::vector<AdMessage>&)> MessageCBFuncT;
    void init(const std::string& ip, uint16_t port);
    void stop();
    bool subscribe(const std::string& topic, MessageCBFuncT on_message);
    void unsubscribe(const std::string& topic, bool remove_retry = true);
    void onAdMessage(const std::vector<AdMessage>& msg_list, int calltype = 0);

private:
    typedef std::map<std::string, MessageCBFuncT>  SubscriberListT;
    void heart_check();
    void resubscribe_all();
    void unsubscribe_all();
    void retry_failed_subscriber();
    void resubscribe(const SubscriberListT& resub_list, bool unsub_before);
    void consume(const std::string& topic);

    SubscriberListT subscriber_list_;
    std::map<std::string, boost::shared_ptr<boost::thread> > consuming_thread_list_;
    std::map<std::string, boost::shared_ptr<izenelib::util::concurrent_queue<AdMessage> > > consume_task_list_;
    boost::shared_ptr<AdStreamReceiveServer> rpcserver_;

    boost::mutex mutex_;
    RpcServerConnection* conn_mgr_;
    SubscriberListT  retry_sub_list_;
    boost::thread    heart_check_thread_;
};

}
#endif

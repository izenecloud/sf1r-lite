#include "AdStreamSubscriber.h"
#include <glog/logging.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <node-manager/SuperNodeManager.h>

#define QUEUE_SIZE 1000

namespace sf1r
{

const AdStreamReceiveServerRequest::method_t AdStreamReceiveServerRequest::method_names[] =
{
    "test",
    "push_admessage",
};

const AdStreamSubscriberServerRequest::method_t AdStreamSubscriberServerRequest::method_names[] =
{
    "test",
    "subscribe_admessage",
};


AdStreamReceiveServer::AdStreamReceiveServer(const std::string& host, uint16_t port, uint32_t threadNum)
    : host_(host)
    , port_(port)
    , threadNum_(threadNum)
{
}

AdStreamReceiveServer::~AdStreamReceiveServer()
{
    LOG(INFO) << "~AdStreamReceiveServer()" << std::endl;
    stop();
}

void AdStreamReceiveServer::start()
{
    instance.listen(host_, port_);
    instance.start(threadNum_);
    LOG(INFO) << "starting ad stream server on : " << host_ << ":" << port_;
}

void AdStreamReceiveServer::join()
{
    instance.join();
}

void AdStreamReceiveServer::run()
{
    start();
    join();
}

void AdStreamReceiveServer::stop()
{
    instance.end();
    instance.join();
}

void AdStreamReceiveServer::dispatch(msgpack::rpc::request req)
{
    try
    {
        std::string method;
        req.method().convert(&method);

        if (method == AdStreamReceiveServerRequest::method_names[AdStreamReceiveServerRequest::METHOD_TEST])
        {
            msgpack::type::tuple<bool> params;
            req.params().convert(&params);
            req.result(true);
        }
        else if (method == AdStreamReceiveServerRequest::method_names[AdStreamReceiveServerRequest::METHOD_PUSH_ADMESSAGE])
        {
            msgpack::type::tuple<AdMessageListData> params;
            req.params().convert(&params);
            AdMessageListData& data_list = params.get<0>();
            AdStreamSubscriber::get()->onAdMessage(data_list.msg_list);
            req.result(true);
        }
        else
        {
            req.error(msgpack::rpc::NO_METHOD_ERROR);
        }
    }
    catch (const msgpack::type_error& e)
    {
        req.error(msgpack::rpc::ARGUMENT_ERROR);
    }
    catch (const std::exception& e)
    {
        req.error(std::string(e.what()));
    }
}

AdStreamSubscriber::AdStreamSubscriber()
    :conn_mgr_(NULL)
{
}

void AdStreamSubscriber::init(const std::string& sub_server_ip, uint16_t sub_server_port)
{
    //rpcserver_.reset(new AdStreamReceiveServer(SuperNodeManager::get()->getLocalHostIP(),
    //        9999, 4));
    rpcserver_.reset(new AdStreamReceiveServer("localhost",
            9999, 4));
    rpcserver_->start();

    conn_mgr_ = new RpcServerConnection();
    RpcServerConnectionConfig config;
    config.rpcThreadNum = 4;
    conn_mgr_->init(config);
    sub_server_ip_ = sub_server_ip;
    sub_server_port_ = sub_server_port;
}

AdStreamSubscriber::~AdStreamSubscriber()
{
    stop();
}

void AdStreamSubscriber::stop()
{
    std::vector<std::string> topic_list;
    {
        boost::unique_lock<boost::mutex> guard(mutex_);
        SubscriberListT::const_iterator sub_it = subscriber_list_.begin();
        while (sub_it != subscriber_list_.end())
        {
            topic_list.push_back(sub_it->first);
            ++sub_it;
        }
    }
    for(size_t i = 0; i < topic_list.size(); ++i)
    {
        unsubscribe(topic_list[i]);
    }

    boost::unique_lock<boost::mutex> guard(mutex_);
    if (rpcserver_)
    {
        rpcserver_->stop();
        rpcserver_.reset();
    }
    delete conn_mgr_;
    conn_mgr_ = NULL;
}

void AdStreamSubscriber::onAdMessage(const std::vector<AdMessage>& msg_list, int calltype)
{
    boost::unique_lock<boost::mutex> guard(mutex_);
    for(size_t i = 0; i < msg_list.size(); ++i)
    {
        SubscriberListT::const_iterator it = subscriber_list_.find(msg_list[i].topic);
        if (it == subscriber_list_.end())
        {
            //LOG(INFO) << "got a message not subscribered : " << msg_list[i].topic;
            continue;
        }
        consume_task_list_[msg_list[i].topic]->push(msg_list[i]);
    }
}

void AdStreamSubscriber::consume(const std::string& topic)
{
    // each topic using the separated consume thread.
    std::string cur_topic = topic;
    LOG(INFO) << "consume thread started for topic: " << cur_topic;

    boost::shared_ptr<izenelib::util::concurrent_queue<AdMessage> > tasks;
    MessageCBFuncT cb_func;
    {
        boost::unique_lock<boost::mutex> lock(mutex_);
        if (subscriber_list_.find(cur_topic) == subscriber_list_.end())
        {
            LOG(INFO) << "no subscriber for cur topic. exit";
            return;
        }
        tasks = consume_task_list_[cur_topic];
        cb_func = subscriber_list_[cur_topic];
    }

    while(true)
    {
        try
        {
            std::vector<AdMessage> msg_list;
            AdMessage msg;
            while(!tasks->empty())
            {
                tasks->pop(msg);
                msg_list.push_back(msg);
            }
            if (msg_list.empty())
                tasks->pop(msg);

            msg_list.push_back(msg);
            boost::this_thread::interruption_point();

            cb_func(msg_list);
        }
        catch(const boost::thread_interrupted& e)
        {
            break;
        }
        catch(const std::exception& e)
        {
            LOG(WARNING) << "consuming exception : " << e.what();
        }
    }
    LOG(INFO) << "consume thread exited for topic: " << cur_topic;
}

bool AdStreamSubscriber::subscribe(const std::string& topic, MessageCBFuncT cb)
{
    if(conn_mgr_ == NULL)
        return false;
    boost::unique_lock<boost::mutex> guard(mutex_);
    SubscriberListT::iterator it = subscriber_list_.find(topic);
    if (it != subscriber_list_.end())
    {
        LOG(INFO) << "topic already subscribed." << topic;
        return true;
    }
    // send subscribe rpc request to server.
    SubscribeAdStreamRequest req;
    req.param_.topic = topic;
    req.param_.ip = SuperNodeManager::get()->getLocalHostIP();
    //req.param_.ip = "localhost";
    req.param_.port = 9999;
    req.param_.un_subscribe = false;
    bool rsp = false;
    try
    {
        //conn_mgr_->syncRequest(sub_server_ip_, sub_server_port_, req, rsp);
        rsp = true;
    }
    catch(const std::exception& e)
    {
        LOG(WARNING) << "send request failed: " << e.what();
        return false;
    }
    if (!rsp)
    {
        LOG(WARNING) << "server return subscribe failed.";
        return false;
    }
    LOG(INFO) << "subscribe topic success: " << topic;

    consume_task_list_[topic].reset(new izenelib::util::concurrent_queue<AdMessage>(QUEUE_SIZE));
    subscriber_list_[topic] = cb; 
    consuming_thread_list_[topic].reset(new boost::thread(boost::bind(&AdStreamSubscriber::consume, this, topic)));
    return true;
}

void AdStreamSubscriber::unsubscribe(const std::string& topic)
{
    LOG(INFO) << "begin unsubscribe topic: " << topic;
    if(conn_mgr_ == NULL)
        return;
    boost::shared_ptr<boost::thread> consume_thread;
    {
        boost::unique_lock<boost::mutex> guard(mutex_);
        SubscriberListT::iterator sub_it = subscriber_list_.find(topic);
        if (sub_it == subscriber_list_.end())
        {
            LOG(INFO) << "unsubscribe topic not found: " << topic;
            return;
        }

        // send unsubscribe rpc request to server.
        SubscribeAdStreamRequest req;
        req.param_.topic = topic;
        req.param_.ip = SuperNodeManager::get()->getLocalHostIP();
        req.param_.port = 9999;
        req.param_.un_subscribe = true;
        bool rsp = false;
        try
        {
            //conn_mgr_->syncRequest(sub_server_ip_, sub_server_port_, req, rsp);
        }
        catch(const std::exception& e)
        {
            LOG(WARNING) << "send request failed: " << e.what();
            return;
        }

        consume_thread = consuming_thread_list_[topic];
        consume_thread->interrupt();
        consume_task_list_[topic]->push(AdMessage());
        subscriber_list_.erase(topic);
        consume_task_list_.erase(topic);
        consuming_thread_list_.erase(topic);
    }
    if (boost::this_thread::get_id() != consume_thread->get_id())
        consume_thread->join();
    LOG(INFO) << "unsubscribe topic success: " << topic;
}

}

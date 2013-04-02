#include "DistributeDriver.h"
#include "MasterManagerBase.h"
#include "DistributeRequestHooker.h"
#include "RequestLog.h"
#include "DistributeTest.hpp"

#include <util/scheduler.h>
#include <util/driver/Reader.h>
#include <util/driver/Writer.h>
#include <util/driver/writers/JsonWriter.h>
#include <util/driver/readers/JsonReader.h>
#include <util/driver/Keys.h>
#include <boost/bind.hpp>
#include <glog/logging.h>

using namespace izenelib::driver;

namespace sf1r
{

DistributeDriver::DistributeDriver()
{
    async_task_worker_ = boost::thread(&DistributeDriver::run, this);
    // only one write task can exist in distribute system.
    asyncWriteTasks_.resize(1);
}

void DistributeDriver::stop()
{
    async_task_worker_.interrupt();
    async_task_worker_.join();
    asyncWriteTasks_.clear();
}

void DistributeDriver::run()
{
    try
    {
        while(true)
        {
            boost::function<bool()> task;
            asyncWriteTasks_.pop(task);
            task();
            boost::this_thread::interruption_point();
        }
    }
    catch (boost::thread_interrupted&)
    {
        // if any request, run it.
        if (!asyncWriteTasks_.empty())
        {
            LOG(INFO) << "running last request before stopping.";
            boost::function<bool()> task;
            asyncWriteTasks_.pop(task);
            task();
        }
        return;
    }
    LOG(ERROR) << "run write thread error.";
    throw -1;
}

void DistributeDriver::init(const RouterPtr& router)
{
    router_ = router;
    MasterManagerBase::get()->setCallback(boost::bind(&DistributeDriver::on_new_req_available, this));
}

static bool callCronJob(Request::kCallType calltype, const std::string& jobname, const std::string& packed_data)
{
    DistributeRequestHooker::get()->processLocalBegin();
    DistributeTestSuit::incWriteRequestTimes("cron_" + jobname);
    bool ret = izenelib::util::Scheduler::runJobImmediatly(jobname, calltype, calltype == Request::FromLog);
    if (!ret)
    {
        LOG(ERROR) << "start cron job failed." << jobname;
    }
    else
    {
        LOG(INFO) << "cron job finished local: " << jobname;
    }
    DistributeRequestHooker::get()->processLocalFinished(ret);
    return ret;
}

static bool callHandler(izenelib::driver::Router::handler_ptr handler,
    Request::kCallType calltype, const std::string& packed_data,
    Request& request)
{
    try
    {
        DistributeRequestHooker::get()->processLocalBegin();
        Response response;
        response.setSuccess(true);
        static Poller tmp_poller;
        // prepare request
        handler->invoke(request,
            response,
            tmp_poller);

        LOG(INFO) << "write request send in DistributeDriver success.";
        return true;
    }
    catch(const std::exception& e)
    {
        LOG(ERROR) << "call request handler exception: " << e.what();
        DistributeRequestHooker::get()->processLocalFinished(false);
        throw;
    }
    return false;
}

static bool callCBWriteHandler(Request::kCallType calltype, const std::string& callback_name,
    const DistributeDriver::CBWriteHandlerT& cb_handler)
{
    DistributeRequestHooker::get()->processLocalBegin();
    DistributeTestSuit::incWriteRequestTimes("callback_" + callback_name);
    bool ret = cb_handler(calltype);
    if (!ret)
    {
        LOG(ERROR) << "callback handler failed." << callback_name;
    }
    else
    {
        LOG(INFO) << "a callback write request finished :" << callback_name;
    }
    DistributeRequestHooker::get()->processLocalFinished(ret);
    return ret;
}

void DistributeDriver::removeCallbackWriteHandler(const std::string& name)
{
    // no lock needed because of this will only happen while stopping collection,
    // it will be sure there is no any callback write request.
    callback_handlers_.erase(name);
    LOG(INFO) << "callback handler removed :" << name;
}

bool DistributeDriver::addCallbackWriteHandler(const std::string& name, const CBWriteHandlerT& handler)
{
    if (callback_handlers_.find(name) != callback_handlers_.end())
    {
        LOG(WARNING) << "callback handler already existed";
        return false;
    }
    if (!handler)
    {
        LOG(WARNING) << "callback NULL handler!";
        return false;
    }
    callback_handlers_[name] = handler;
    return true;
}

bool DistributeDriver::handleRequest(const std::string& reqjsondata, const std::string& packed_data, Request::kCallType calltype)
{
    static izenelib::driver::JsonReader reader;
    Value requestValue;
    if(reader.read(reqjsondata, requestValue))
    {
        if (requestValue.type() != Value::kObjectType)
        {
            LOG(ERROR) << "read request data type error: Malformed request: require an object as input.";
            return false;
        }
        Request request;
        request.assignTmp(requestValue);
        izenelib::driver::Router::handler_ptr handler = router_->find(
            request.controller(),
            request.action()
            );
        if (!handler)
        {
            LOG(ERROR) << "Handler not found for the request : " << request.controller() <<
                "," << request.action();
            return false;
        }
        if (!ReqLogMgr::isWriteRequest(request.controller(), request.action()))
        {
            LOG(ERROR) << "=== Wrong, not a write request in DistributeDriver!!";
            return false;
        }
        try
        {
            if (!asyncWriteTasks_.empty())
            {
                LOG(ERROR) << "another write task is running in async_task_worker_!!";
                return false;
            }

            DistributeTestSuit::incWriteRequestTimes(request.controller() + "_" + request.action());

            request.setCallType(calltype);

            if (calltype == Request::FromLog)
            {
                // redo log must process the request one by one, so sync needed.
                DistributeRequestHooker::get()->setHook(calltype, packed_data);
                DistributeRequestHooker::get()->hookCurrentReq(packed_data);
                return callHandler(handler, calltype, packed_data, request);
            }
            else
            {
                if (!async_task_worker_.interruption_requested())
                {
                    DistributeRequestHooker::get()->setHook(calltype, packed_data);
                    DistributeRequestHooker::get()->hookCurrentReq(packed_data);
                    asyncWriteTasks_.push(boost::bind(&callHandler,
                            handler, calltype, packed_data, request));
                }
                else
                {
                    LOG(INFO) << "write request has been interrupt.";
                    return false;
                }
            }
            return true;
        }
        catch (const std::exception& e)
        {
            LOG(ERROR) << "process request exception: " << e.what();
        }
    }
    else
    {
        // malformed request
        LOG(WARNING) << "read request data error: " << reader.errorMessages();
    }
    return false;
}

bool DistributeDriver::handleReqFromLog(int reqtype, const std::string& reqjsondata, const std::string& packed_data)
{
    if ((ReqLogType)reqtype == Req_CronJob)
    {
        if (!asyncWriteTasks_.empty())
        {
            LOG(ERROR) << "another write task is running in async_task_worker_!!";
            return false;
        }

	    LOG(INFO) << "got a cron job request in log." << reqjsondata;
	    DistributeRequestHooker::get()->setHook(Request::FromLog, packed_data);
        DistributeRequestHooker::get()->hookCurrentReq(packed_data);

	    return callCronJob(Request::FromLog, reqjsondata, packed_data);
    }
    else if ((ReqLogType)reqtype == Req_Callback)
    {
        if (!asyncWriteTasks_.empty())
        {
            LOG(ERROR) << "another write task is running in async_task_worker_!!";
            return false;
        }

	    LOG(INFO) << "got a callback request in log." << reqjsondata;
        std::map<std::string, CBWriteHandlerT>::const_iterator it = callback_handlers_.find(reqjsondata);
        if (it == callback_handlers_.end())
        {
            LOG(WARNING) << "callback write request not found on the node: " << reqjsondata;
            return false;
        }

	    DistributeRequestHooker::get()->setHook(Request::FromLog, packed_data);
        DistributeRequestHooker::get()->hookCurrentReq(packed_data);
	    return callCBWriteHandler(Request::FromLog, reqjsondata, it->second);
    }

    return handleRequest(reqjsondata, packed_data, Request::FromLog);
}

bool DistributeDriver::handleReqFromPrimary(int reqtype, const std::string& reqjsondata, const std::string& packed_data)
{
    if ((ReqLogType)reqtype == Req_CronJob)
    {
        LOG(INFO) << "got a cron job request from primary." << reqjsondata;

        if (!async_task_worker_.interruption_requested())
        {
            DistributeRequestHooker::get()->setHook(Request::FromPrimaryWorker, packed_data);
            DistributeRequestHooker::get()->hookCurrentReq(packed_data);
            asyncWriteTasks_.push(boost::bind(&callCronJob,
                    Request::FromPrimaryWorker, reqjsondata, packed_data));
        }
        else
        {
            LOG(INFO) << "write task has been interrupt : " << reqjsondata;
            return false;
        }
        return true;
    }
    else if((ReqLogType)reqtype == Req_Callback)
    {
	    LOG(INFO) << "got a callback request from primary." << reqjsondata;

        std::map<std::string, CBWriteHandlerT>::const_iterator it = callback_handlers_.find(reqjsondata);
        if (it == callback_handlers_.end())
        {
            LOG(WARNING) << "callback write request not found on the node: " << reqjsondata;
            return false;
        }

        if (!async_task_worker_.interruption_requested())
        {
            DistributeRequestHooker::get()->setHook(Request::FromPrimaryWorker, packed_data);
            DistributeRequestHooker::get()->hookCurrentReq(packed_data);
            asyncWriteTasks_.push(boost::bind(&callCBWriteHandler,
                    Request::FromPrimaryWorker, reqjsondata, it->second));
        }
        else
        {
            LOG(INFO) << "write task has been interrupt : " << reqjsondata;
            return false;
        }
        return true;
    }

    return handleRequest(reqjsondata, packed_data, Request::FromPrimaryWorker);
}

bool DistributeDriver::pushCallbackWrite(const std::string& name, const std::string& packed_data)
{
    LOG(INFO) << "a callback write pushed to queue: " << name;
    MasterManagerBase::get()->pushWriteReq(name + "::" + packed_data, "callback");
    return true;
}

bool DistributeDriver::on_new_req_available()
{
    if (!MasterManagerBase::get()->prepareWriteReq())
    {
        LOG(WARNING) << "prepare new request failed. maybe some other primary master prepared first. ";
        return false;
    }
    while(true)
    {
        std::string reqdata;
        std::string reqtype;
        if(!MasterManagerBase::get()->popWriteReq(reqdata, reqtype))
        {
            LOG(INFO) << "no more request.";
            return false;
        }

        if(reqtype.empty() && !handleRequest(reqdata, reqdata, Request::FromDistribute) )
        {
            LOG(WARNING) << "one write request failed to deliver :" << reqdata;
            // a write request failed to deliver means the worker did not 
            // started to handle this request, so the request is ignored, and
            // continue to deliver next write request in the queue.
        }
        else if (reqtype == "api_from_shard")
        {
            if(!handleRequest(reqdata, reqdata, Request::FromOtherShard))
            {
                LOG(WARNING) << "one api write request from shard failed to deliver :" << reqdata;
            }
            else
            {
                break;
            }
        }
        else if (reqtype == "cron")
        {
            LOG(INFO) << "got a cron job request from queue." << reqdata;
            if (!async_task_worker_.interruption_requested())
            {
                DistributeRequestHooker::get()->setHook(Request::FromDistribute, reqdata);
                DistributeRequestHooker::get()->hookCurrentReq(reqdata);
                asyncWriteTasks_.push(boost::bind(&callCronJob,
                        Request::FromDistribute, reqdata, reqdata));
                break;
            }
            else
            {
                return false;
            }
        }
        else if (reqtype == "callback")
        {
            LOG(INFO) << "got a callback write request : " << reqdata;
            // get the callback name and packed_data.
            size_t split_pos = reqdata.find("::");
            if (split_pos == std::string::npos)
            {
                LOG(WARNING) << "callback data invalid.";
                continue;
            }
            std::string callback_name = reqdata.substr(0, split_pos);
            std::string packed_data = reqdata.substr(split_pos + 2);
            std::map<std::string, CBWriteHandlerT>::const_iterator it = callback_handlers_.find(callback_name);
            if (it == callback_handlers_.end())
            {
                LOG(WARNING) << "callback write request not found on the node: " << callback_name;
                continue;
            }
            if (!async_task_worker_.interruption_requested())
            {
                DistributeRequestHooker::get()->setHook(Request::FromOtherShard, packed_data);
                DistributeRequestHooker::get()->hookCurrentReq(packed_data);

                asyncWriteTasks_.push(boost::bind(&callCBWriteHandler, Request::FromOtherShard,
                        callback_name, it->second));
                break;
            }
            else
            {
                return false;
            }
        }
        else
        {
            // a write request deliver success from master to worker.
            // this mean the worker has started to process this request,
            // may be not finish writing, so we break here to wait the finished
            // event from primary worker.
            break;
        }
    }
    return true;
}

}

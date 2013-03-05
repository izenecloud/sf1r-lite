#include "DistributeDriver.h"
#include "MasterManagerBase.h"
#include "DistributeRequestHooker.h"
#include "RequestLog.h"

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
        return;
    }
}

void DistributeDriver::init(const RouterPtr& router)
{
    router_ = router;
    MasterManagerBase::get()->setCallback(boost::bind(&DistributeDriver::on_new_req_available, this));
}

static bool callCronJob(Request::kCallType calltype, const std::string& jobname, const std::string& packed_data)
{
	bool ret = izenelib::util::Scheduler::runJobImmediatly(jobname, calltype, calltype == Request::FromLog);
	if (!ret)
	{
		LOG(ERROR) << "start cron job failed." << jobname;
		DistributeRequestHooker::get()->processLocalFinished(false);
	}
	return ret;
}

static bool callHandler(izenelib::driver::Router::handler_ptr handler,
    Request::kCallType calltype, const std::string& packed_data,
    Request& request)
{
    try
    {
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
        throw e;
    }
    return false;
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

            DistributeRequestHooker::get()->setHook(calltype, packed_data);
            request.setCallType(calltype);
            DistributeRequestHooker::get()->hookCurrentReq(packed_data);
            DistributeRequestHooker::get()->processLocalBegin();

            if (calltype == Request::FromLog)
            {
                // redo log must process the request one by one, so sync needed.
                return callHandler(handler, calltype, packed_data, request);
            }
            else
            {
                if (!async_task_worker_.interruption_requested())
                {
                    asyncWriteTasks_.push(boost::bind(&callHandler,
                            handler, calltype, packed_data, request));
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
        DistributeRequestHooker::get()->processLocalBegin();

	    return callCronJob(Request::FromLog, reqjsondata, packed_data);
    }
    return handleRequest(reqjsondata, packed_data, Request::FromLog);
}

bool DistributeDriver::handleReqFromPrimary(int reqtype, const std::string& reqjsondata, const std::string& packed_data)
{
    if ((ReqLogType)reqtype == Req_CronJob)
    {
        LOG(INFO) << "got a cron job request from primary." << reqjsondata;
        DistributeRequestHooker::get()->setHook(Request::FromPrimaryWorker, packed_data);
        DistributeRequestHooker::get()->hookCurrentReq(packed_data);
        DistributeRequestHooker::get()->processLocalBegin();

        if (!async_task_worker_.interruption_requested())
        {
            asyncWriteTasks_.push(boost::bind(&callCronJob,
                    Request::FromPrimaryWorker, reqjsondata, packed_data));
        }
        return true;
    }

    return handleRequest(reqjsondata, packed_data, Request::FromPrimaryWorker);
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
            break;
        }

        if(reqtype.empty() && !handleRequest(reqdata, reqdata, Request::FromDistribute) )
        {
            LOG(WARNING) << "one write request failed to deliver :" << reqdata;
            // a write request failed to deliver means the worker did not 
            // started to handle this request, so the request is ignored, and
            // continue to deliver next write request in the queue.
        }
        else if (reqtype == "cron")
        {
            LOG(INFO) << "got a cron job request from queue." << reqdata;
            DistributeRequestHooker::get()->setHook(Request::FromDistribute, reqdata);
            DistributeRequestHooker::get()->hookCurrentReq(reqdata);
            DistributeRequestHooker::get()->processLocalBegin();

            if (!async_task_worker_.interruption_requested())
            {
                asyncWriteTasks_.push(boost::bind(&callCronJob,
                        Request::FromDistribute, reqdata, reqdata));
            }
            break;
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

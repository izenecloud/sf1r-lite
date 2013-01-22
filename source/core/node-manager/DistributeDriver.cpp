#include "DistributeDriver.h"
#include "SearchMasterManager.h"
#include "RecommendMasterManager.h"
#include "DistributeRequestHooker.h"
#include "RequestLog.h"

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
            boost::function<void()> task;
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
    SearchMasterManager::get()->setCallback(boost::bind(&DistributeDriver::on_new_req_available, this));
}

static void callHandler(izenelib::driver::Router::handler_ptr handler,
    Request::kCallType calltype, const std::string& packed_data,
    Request& request)
{
    try
    {
        static Response response;
        static Poller tmp_poller;
        // prepare request
        handler->invoke(request,
            response,
            tmp_poller);
        LOG(INFO) << "write request send in DistributeDriver success.";

        DistributeRequestHooker::get()->clearHook();
    }
    catch(const std::exception& e)
    {
        LOG(ERROR) << "call request handler exception: " << e.what();
    }
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
            DistributeRequestHooker::get()->setHook(calltype, packed_data);
            request.setCallType(calltype);
            if (calltype == Request::FromLog)
            {
                if (!asyncWriteTasks_.empty())
                {
                    LOG(ERROR) << "another write task is running in async_task_worker_!!";
                    return false;
                }
                // redo log must process the request one by one, so sync needed.
                callHandler(handler, calltype, packed_data, request);
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

bool DistributeDriver::handleReqFromLog(const std::string& reqjsondata, const std::string& packed_data)
{
    return handleRequest(reqjsondata, packed_data, Request::FromLog);
}

bool DistributeDriver::handleReqFromPrimary(const std::string& reqjsondata, const std::string& packed_data)
{
    return handleRequest(reqjsondata, packed_data, Request::FromPrimaryWorker);
}

bool DistributeDriver::on_new_req_available()
{
    if (!SearchMasterManager::get()->prepareWriteReq())
    {
        LOG(WARNING) << "prepare new request failed. maybe some other primary master prepared first. ";
        return false;
    }
    std::string reqdata;
    if(!SearchMasterManager::get()->popWriteReq(reqdata))
    {
        LOG(INFO) << "pop request data failed.";
        return false;
    }
    return handleRequest(reqdata, reqdata, Request::FromDistribute);
}

}

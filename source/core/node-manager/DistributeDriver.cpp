#include "DistributeDriver.h"
#include "SearchMasterManager.h"
#include "RecommendMasterManager.h"

#include <util/driver/writers/JsonWriter.h>
#include <util/driver/readers/JsonReader.h>
#include <util/driver/Keys.h>
#include <boost/bind.hpp>
#include <glog/logging.h>

using namespace izenelib::driver;

namespace sf1r
{

void DistributeDriver::init(const RouterPtr& router)
{
    router_ = router;
    SearchMasterManager::get()->setCallback(boost::bind(&DistributeDriver::on_new_req_available, this));
}

void DistributeDriver::on_new_req_available()
{
    if (!SearchMasterManager::get()->prepareWriteReq())
    {
        LOG(WARNING) << "prepare new request failed. maybe some other primary master prepared first. ";
        return;
    }
    std::string reqdata;
    if(!SearchMasterManager::get()->popWriteReq(reqdata))
    {
        LOG(INFO) << "pop request data failed.";
        return;
    }
    static izenelib::driver::JsonReader reader;
    Value requestValue;
    if(reader.read(reqdata, requestValue))
    {
        if (requestValue.type() != Value::kObjectType)
        {
            LOG(ERROR) << "read request data type error: Malformed request: require an object as input.";
            return;
        }
        Request request;
        static Response response;
        static Poller tmp_poller;
        request.assignTmp(requestValue);
        izenelib::driver::Router::handler_ptr handler = router_->find(
            request.controller(),
            request.action()
            );
        if (!handler)
        {
            LOG(ERROR) << "Handler not found for the request : " << request.controller() <<
                "," << request.action();
            return;
        }
        try
        {
            // prepare request
            request.setCallType(Request::FromDistribute);
            handler->invoke(request,
                response,
                tmp_poller);
            LOG(INFO) << "write request send from primary master success.";
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
}

}

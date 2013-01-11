#include "DistributeRequestHooker.h"

namespace sf1r
{

void DistributeRequestHooker::setCurrentReq(const std::string& reqdata)
{
    current_req_ = reqdata;
}

bool DistributeRequestHooker::isHooked()
{
    return !current_req_.empty();
}

bool DistributeRequestHooker::backup()
{
    return true;
}

void DistributeRequestHooker::processLocalBegin()
{
}

void DistributeRequestHooker::processLocalFinished()
{
}

bool DistributeRequestHooker::process2Replicas()
{
    return true;
}

bool DistributeRequestHooker::waitReplicasProcess()
{
    return true;
}

bool DistributeRequestHooker::waitPrimary()
{
    return true;
}

void DistributeRequestHooker::abortRequest()
{
}

bool DistributeRequestHooker::waitReplicasAbort()
{
    return true;
}

bool DistributeRequestHooker::writeLocalLog()
{
    return true;
}

void DistributeRequestHooker::writeLog2Replicas()
{
}

bool DistributeRequestHooker::waitReplicasLog()
{
    return true;
}

void DistributeRequestHooker::finish()
{
}

}


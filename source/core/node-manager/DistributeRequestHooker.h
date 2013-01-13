#ifndef SF1R_DISTRIBUTE_REQUEST_HOOKER_H
#define SF1R_DISTRIBUTE_REQUEST_HOOKER_H

#include "RequestLog.h"
#include <string>
#include <configuration-manager/CollectionPath.h>

namespace sf1r
{
class ReqLogMgr;
class DistributeRequestHooker
{
public:
    DistributeRequestHooker();
    void hookCurrentReq(const std::string& colname, const CollectionPath& colpath,
        const std::string& reqdata, boost::shared_ptr<ReqLogMgr> req_log_mgr);
    bool isHooked();

    bool prepare(ReqLogType type, CommonReqData& prepared_req);
    void processLocalBegin();
    void processLocalFinished(bool finishsuccess, const std::string& packed_reqdata);
    void onRequestFromPrimary(int type, const std::string& packed_reqdata);

    bool waitReplicasProcessCallback();
    bool waitPrimaryCallback();

    void abortRequest();
    void abortRequestCallback();
    bool waitReplicasAbortCallback();

    bool writeLocalLog();
    void writeLog2Replicas();
    bool waitReplicasLogCallback();

    void onElectingFinished();
    void finish();

private:
    void forceExit();
    std::string colname_;
    CollectionPath colpath_;
    std::string current_req_;
    ReqLogType type_;
    boost::shared_ptr<ReqLogMgr> req_log_mgr_;
};

}

#endif

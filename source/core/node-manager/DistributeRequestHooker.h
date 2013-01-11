#ifndef SF1R_DISTRIBUTE_REQUEST_HOOKER_H
#define SF1R_DISTRIBUTE_REQUEST_HOOKER_H

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

    bool prepare();
    void processLocalBegin();
    void processLocalFinished();

    bool process2Replicas();
    bool waitReplicasProcessCallback();
    bool waitPrimaryCallback();

    void abortRequest();
    bool waitReplicasAbortCallback();

    bool writeLocalLog();
    void writeLog2Replicas();
    bool waitReplicasLogCallback();
    void finish();

private:
    std::string colname_;
    CollectionPath colpath_;
    std::string current_req_;
    boost::shared_ptr<ReqLogMgr> req_log_mgr_;
};

}

#endif

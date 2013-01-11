#ifndef SF1R_DISTRIBUTE_REQUEST_HOOKER_H
#define SF1R_DISTRIBUTE_REQUEST_HOOKER_H

#include <string>

namespace sf1r
{
class DistributeRequestHooker
{
public:
    void setCurrentReq(const std::string& reqdata);
    bool isHooked();

    bool backup();
    void processLocalBegin();
    void processLocalFinished();

    bool process2Replicas();
    bool waitReplicasProcess();
    bool waitPrimary();

    void abortRequest();
    bool waitReplicasAbort();

    bool writeLocalLog();
    void writeLog2Replicas();
    bool waitReplicasLog();
    void finish();

private:
    std::string current_req_;
};

}

#endif

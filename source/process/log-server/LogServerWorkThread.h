/**
 * @file LogServerWorkThread.h
 * @author Zhongxia Li
 * @date Jan 11, 2012
 * @brief
 */
#ifndef LOG_SERVER_WORK_THREAD_H_
#define LOG_SERVER_WORK_THREAD_H_

#include <log-manager/LogServerRequestData.h>
#include <util/concurrent_queue.h>
#include <util/singleton.h>

#include <boost/thread.hpp>

namespace sf1r
{

class LogServerWorkThread
{
    struct DrumRequestData
    {
        boost::shared_ptr<UUID2DocidList> uuid2DocidList;
        boost::shared_ptr<SynchronizeData> syncReqData;
    };

public:
    LogServerWorkThread();

    ~LogServerWorkThread();

    void stop();

    void putUuidRequestData(const UUID2DocidList& uuid2DocidList);

    bool idle();
    void putSyncRequestData(const SynchronizeData& syncReqData);

private:
    void run();

    void monitor();

    void process(const DrumRequestData& drumReqData);

    void process(const UUID2DocidList& uuid2DocidList);

    void process(const SynchronizeData& syncReqData);

    void flush();

private:
    boost::posix_time::ptime lastProcessTime_;
    boost::posix_time::ptime lastCheckedTime_;
    long monitorInterval_; // in seconds

    izenelib::util::concurrent_queue<DrumRequestData> drumRequestQueue_;

    boost::thread workThread_;
    boost::thread monitorThread_;
};

}

#endif /* LOG_SERVER_WORK_THREAD_H_ */

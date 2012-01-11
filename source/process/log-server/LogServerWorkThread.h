/**
 * @file LogServerWorkThread.h
 * @author Zhongxia Li
 * @date Jan 11, 2012
 * @brief 
 */
#ifndef LOG_SERVER_WORK_THREAD_H_
#define LOG_SERVER_WORK_THREAD_H_

#include "LogServerStorage.h"

#include <log-manager/LogServerRequestData.h>

#include <util/concurrent_queue.h>
#include <util/singleton.h>

#include <boost/thread.hpp>

namespace sf1r
{

class LogServerWorkThread
{
public:
    LogServerWorkThread(
            const LogServerStorage::DrumPtr& drum,
            const LogServerStorage::KVDBPtr& docidDB);

    ~LogServerWorkThread();

    void stop();

    void putUuidRequestData(const UUID2DocidList& uuid2DocidList);

    void putSyncRequestData(const SynchronizeData& syncReqData);

private:
    void run();

    void process(const UUID2DocidList& uuid2DocidList);

    void process(const SynchronizeData& syncReqData);

private:
    boost::thread workThread_;

    LogServerStorage::DrumPtr drum_;
    LogServerStorage::KVDBPtr docidDB_;

    izenelib::util::concurrent_queue<UUID2DocidList> uuidRequestQueue_;
    izenelib::util::concurrent_queue<SynchronizeData> syncRequestQueue_;
};

}

#endif /* LOG_SERVER_WORK_THREAD_H_ */

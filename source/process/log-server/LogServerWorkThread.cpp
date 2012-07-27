#include "LogServerWorkThread.h"
#include "LogServerStorage.h"
#include <ctime>

#include <glog/logging.h>

namespace sf1r
{

LogServerWorkThread::LogServerWorkThread()
    : monitorInterval_(LogServerCfg::get()->getFlushCheckInterval())
    , drumRequestQueue_(LogServerCfg::get()->getRpcRequestQueueSize())
    , workThread_(&LogServerWorkThread::run, this)
    , monitorThread_(&LogServerWorkThread::monitor, this)
{
}

LogServerWorkThread::~LogServerWorkThread()
{
    stop();
}

void LogServerWorkThread::stop()
{
    workThread_.interrupt();
    workThread_.join();

    monitorThread_.interrupt();
    monitorThread_.join();
}

void LogServerWorkThread::putUuidRequestData(const UUID2DocidList& uuid2DocidList)
{
    DrumRequestData drumReqData;

    drumReqData.uuid2DocidList.reset(new UUID2DocidList(uuid2DocidList));
    drumRequestQueue_.push(drumReqData);
}

void LogServerWorkThread::putSyncRequestData(const SynchronizeData& syncReqData)
{
    DrumRequestData drumReqData;

    drumReqData.syncReqData.reset(new SynchronizeData(syncReqData));
    drumRequestQueue_.push(drumReqData);
}

bool LogServerWorkThread::idle()
{
    return drumRequestQueue_.empty();
}

void LogServerWorkThread::run()
{
    try
    {
        DrumRequestData drumReqData;

        while (true)
        {
            drumRequestQueue_.pop(drumReqData);
            process(drumReqData);

            if (drumRequestQueue_.empty())
            {
                drumRequestQueue_.clear();
            }

            lastProcessTime_ = boost::posix_time::second_clock::universal_time();

            // terminate execution if interrupted
            boost::this_thread::interruption_point();
        }
    }
    catch (const std::exception& e)
    {
    }
}

void LogServerWorkThread::monitor()
{
    try
    {
        while (true)
        {
            boost::this_thread::sleep(boost::posix_time::seconds(monitorInterval_));

            // try to release memory while work thread was blocked for a period of time waiting for requests.
            // (if request for flush was not called, memory would not be released)
            if (lastProcessTime_.is_not_a_date_time() == false && lastCheckedTime_ != lastProcessTime_)
            {
                boost::posix_time::ptime now_time = boost::posix_time::second_clock::universal_time();
                boost::posix_time::time_duration time_elapse = now_time - lastProcessTime_;
                if (time_elapse.total_seconds() > monitorInterval_)
                {
                    std::cout << "=> try to flush data after " << time_elapse.total_seconds()
                              << " seconds since processed last request." << std::endl;

                    lastCheckedTime_ = lastProcessTime_;

                    // add request to flush data
                    DrumRequestData drumReqData;
                    drumReqData.syncReqData.reset(new SynchronizeData);
                    drumRequestQueue_.push(drumReqData);
                }
            }
        }
    }
    catch (const std::exception& e)
    {
    }
}

void LogServerWorkThread::process(const DrumRequestData& drumReqData)
{
    if (drumReqData.uuid2DocidList)
    {
        process(*drumReqData.uuid2DocidList);
    }

    if (drumReqData.syncReqData)
    {
        process(*drumReqData.syncReqData);
    }
}

void LogServerWorkThread::process(const UUID2DocidList& uuid2DocidList)
{
    boost::lock_guard<boost::mutex> lock(LogServerStorage::get()->uuidDrumMutex());
    LogServerStorage::get()->uuidDrum()->Update(uuid2DocidList.uuid_, uuid2DocidList.docidList_);
}

void LogServerWorkThread::process(const SynchronizeData& syncReqData)
{
    flushData();
}

void LogServerWorkThread::flushData()
{
    {
        boost::lock_guard<boost::mutex> lock(LogServerStorage::get()->uuidDrumMutex());
        LOG(INFO) << "synchronizing drum for uuids (locked)";
        LogServerStorage::get()->uuidDrum()->Synchronize();
        LOG(INFO) << "finished synchronizing drum for uuids";
    }

    {
        boost::lock_guard<boost::mutex> lock(LogServerStorage::get()->docidDrumMutex());
        LOG(INFO) << "synchronizing drum for docids (locked)";
        LogServerStorage::get()->docidDrum()->Synchronize();
        LOG(INFO) << "finished synchronizing drum for docids";
    }
    {
        boost::lock_guard<boost::mutex> lock(LogServerStorage::get()->historyDBMutex());
        LogServerStorage::get()->historyDB()->flush();
    }
    LogServerStorage::get()->close();
    LogServerStorage::get()->init();
}

}

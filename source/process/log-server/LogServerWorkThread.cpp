#include "LogServerWorkThread.h"

#include <glog/logging.h>

namespace sf1r
{

LogServerWorkThread::LogServerWorkThread()
    : workThread_(&LogServerWorkThread::run, this)
    , drumRequestQueue_(LogServerCfg::get()->getRpcRequestQueueSize())
{
}

LogServerWorkThread::~LogServerWorkThread()
{
}

void LogServerWorkThread::stop()
{
    workThread_.interrupt();
    workThread_.join();
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

void LogServerWorkThread::run()
{
    try
    {
        DrumRequestData drumReqData;
        while (true)
        {
            // process requests
            drumRequestQueue_.pop(drumReqData);
            process(drumReqData);

            // terminate execution if interrupted
            boost::this_thread::interruption_point();
        }
    }
    catch (const std::exception& e)
    {
        // nothing
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
    {
        boost::lock_guard<boost::mutex> lock(LogServerStorage::get()->uuidDrumMutex());
        std::cout << "[LogServerWorkThread] synchronizing drum (locked) " << std::endl;
        LogServerStorage::get()->uuidDrum()->Synchronize();
        std::cout << "[LogServerWorkThread] finished synchronizing drum " << std::endl;
    }

    {
        boost::lock_guard<boost::mutex> lock(LogServerStorage::get()->docidDBMutex());
        std::cout << "[LogServerWorkThread] flushing docid DB (locked) " << std::endl;
        LogServerStorage::get()->docidDB()->flush();
        std::cout << "[LogServerWorkThread] finished flushing docid DB " << std::endl;
    }
}

}

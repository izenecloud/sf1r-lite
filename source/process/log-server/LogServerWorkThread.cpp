#include "LogServerWorkThread.h"

namespace sf1r
{

LogServerWorkThread::LogServerWorkThread(
        const LogServerStorage::DrumPtr& drum,
        const LogServerStorage::KVDBPtr& docidDB)
    : drum_(drum)
    , docidDB_(docidDB)
{
    workThread_ = boost::thread(&LogServerWorkThread::run, this);
}

LogServerWorkThread::~LogServerWorkThread()
{
    stop();
}

void LogServerWorkThread::stop()
{
    workThread_.interrupt();
    workThread_.join();
}

void LogServerWorkThread::putUuidRequestData(const UUID2DocidList& uuid2DocidList)
{
    uuidRequestQueue_.push(uuid2DocidList);
}

void LogServerWorkThread::putSyncRequestData(const SynchronizeData& syncReqData)
{
    syncRequestQueue_.push(syncReqData);
}

void LogServerWorkThread::run()
{
    try
    {
        UUID2DocidList uuid2DocidList;
        SynchronizeData syncReqData;
        while (true)
        {
            // tasks
            uuidRequestQueue_.pop(uuid2DocidList);
            process(uuid2DocidList);

            if (!syncRequestQueue_.empty()) // do not wait here
            {
                syncRequestQueue_.pop(syncReqData);
                process(syncReqData);
            }

            // terminate execution if interrupted
            boost::this_thread::interruption_point();

            boost::this_thread::sleep(boost::posix_time::seconds(1));
        }
    }
    catch (const std::exception& e)
    {
        // nothing
    }
}

void LogServerWorkThread::process(const UUID2DocidList& uuid2DocidList)
{
    boost::lock_guard<boost::mutex> lock(LogServerStorage::get()->drumMutex());
    drum_->Update(uuid2DocidList.uuid_, uuid2DocidList.docidList_);
}

void LogServerWorkThread::process(const SynchronizeData& syncReqData)
{
    std::cout << "LogServerWorkThread::processSyncRequest" << std::endl;

    {
        boost::lock_guard<boost::mutex> lock(LogServerStorage::get()->drumMutex());
        drum_->Synchronize();
    }

    {
        boost::lock_guard<boost::mutex> lock(LogServerStorage::get()->docidDBMutex());
        docidDB_->flush();
    }
}

}

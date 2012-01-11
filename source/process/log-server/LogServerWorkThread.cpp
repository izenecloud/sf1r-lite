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

            //boost::this_thread::sleep(boost::posix_time::seconds(1));

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

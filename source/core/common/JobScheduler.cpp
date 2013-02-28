#include "JobScheduler.h"

namespace sf1r
{

JobScheduler::JobScheduler()
{
    wait_event_id_ = 0;
    asynchronousWorker_ = boost::thread(
        &JobScheduler::runAsynchronousTasks_, this
    );
}

JobScheduler::~JobScheduler()
{
    close();
}

void JobScheduler::close()
{
    asynchronousWorker_.interrupt();
    if (boost::this_thread::get_id() == asynchronousWorker_.get_id())
        return;
    asynchronousWorker_.join();
}

void JobScheduler::notifyFinish(int waitid)
{
    boost::unique_lock<boost::mutex> lk(mutex_);
    wait_finish_events_[waitid] = true;
    cond_.notify_all();
}

void JobScheduler::waitCurrentFinish(const std::string& collection)
{
    if (boost::this_thread::get_id() == asynchronousWorker_.get_id())
        return;
    int current_id = 0;
    {
        boost::unique_lock<boost::mutex> lk(mutex_);
        current_id = ++wait_event_id_;
        wait_finish_events_[current_id] = false;
    }
    asynchronousTasks_.push(task_collection_name_type(boost::bind(&JobScheduler::notifyFinish, this, current_id), collection));

    boost::unique_lock<boost::mutex> lk(mutex_);
    while (!wait_finish_events_[current_id])
        cond_.wait(lk);
}

void JobScheduler::removeTask(const std::string& collection)
{
    RemoveTaskPred removePred(collection);
    IsOtherCollection isOtherCollection(currentTaskCollection_, collection);

    if (!asynchronousTasks_.remove_if_when(removePred, isOtherCollection))
    {
        // as there is a running task of the same "collection",
        // it is necessary to wait for its completion.
        close();

        asynchronousTasks_.remove_if(removePred);
        asynchronousWorker_ = boost::thread(
            &JobScheduler::runAsynchronousTasks_, this);
    }
}

void JobScheduler::addTask(task_type task, const std::string& collection)
{
    task_collection_name_type taskCollectionPair(task, collection);
    asynchronousTasks_.push(taskCollectionPair);
}

void JobScheduler::setCapacity(std::size_t capacity)
{
    asynchronousTasks_.resize(capacity);
}

void JobScheduler::runAsynchronousTasks_()
{
    try
    {
        while (true)
        {
            asynchronousTasks_.pop(currentTaskCollection_);

            (currentTaskCollection_.first)();

            // terminate execution if interrupted
            boost::this_thread::interruption_point();
        }
    }
    catch (boost::thread_interrupted&)
    {
        return;
    }
}

}

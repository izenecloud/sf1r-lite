#include "JobScheduler.h"

namespace sf1r
{

JobScheduler::JobScheduler()
{
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
    asynchronousWorker_.join();
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

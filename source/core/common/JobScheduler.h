#ifndef PROCESS_JOB_SCHEDULER_H
#define PROCESS_JOB_SCHEDULER_H

#include <util/concurrent_queue.h>
#include <util/singleton.h>

#include <boost/thread.hpp>
#include <boost/function.hpp>

typedef boost::function0<void> task_type;

namespace sf1r
{

class JobScheduler
{
public:
    typedef std::pair<task_type, std::string> task_collection_name_type;

    struct RemoveTaskPred;

    struct IsOtherCollection;

    JobScheduler();

    ~JobScheduler();

    static JobScheduler* get()
    {
        return ::izenelib::util::Singleton<JobScheduler>::get();
    }

    void addTask(task_type task, const std::string& collection = "");

    void close();

    void removeTask(const std::string& collection);

    void setCapacity(std::size_t capacity);
    void waitCurrentFinish(const std::string& collection = "");

private:
    void runAsynchronousTasks_();
    void notifyFinish(int waitid);

private:
    ::izenelib::util::concurrent_queue<task_collection_name_type> asynchronousTasks_;

    boost::thread asynchronousWorker_;

    task_collection_name_type currentTaskCollection_;
    boost::mutex mutex_;
    boost::condition_variable cond_;
    std::map<int, bool>  wait_finish_events_;
    int wait_event_id_;
};

struct JobScheduler::RemoveTaskPred
{
    const std::string& collection_;

    RemoveTaskPred(const std::string& collection)
        :collection_(collection)
    {
    }

    bool operator()(const task_collection_name_type& task_collection) const
    {
        return collection_ == task_collection.second;
    }
};

struct JobScheduler::IsOtherCollection
{
    const task_collection_name_type& current_task_collection_;

    const std::string& collection_;

    IsOtherCollection(
        const task_collection_name_type& current_task_collection,
        const std::string& collection)
        :current_task_collection_(current_task_collection)
        ,collection_(collection)
    {
    }

    bool operator()() const
    {
        return collection_ != current_task_collection_.second;
    }
};

}

#endif

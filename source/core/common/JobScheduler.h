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

    JobScheduler();

    ~JobScheduler();

    static JobScheduler* get()
    {
        return ::izenelib::util::Singleton<JobScheduler>::get();
    }

    struct RemoveTaskPred
    {
        std::string collection_;

        RemoveTaskPred(const std::string collection)
            :collection_(collection)
        {
        }

        bool operator()(const task_collection_name_type& task_collection) const
        {
            return collection_ == task_collection.second;
        }
    };
    void addTask(task_type task, const std::string& collection = "");

    void close();

    void removeTask(const std::string& collection);

    void setCapacity(std::size_t capacity);

private:
    void runAsynchronousTasks_();

private:
    ::izenelib::util::concurrent_queue<task_collection_name_type> asynchronousTasks_;

    boost::thread asynchronousWorker_;

    std::string runCollectionName_;
};

}

#endif

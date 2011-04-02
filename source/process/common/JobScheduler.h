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
    JobScheduler();

    ~JobScheduler();

    static JobScheduler* get()
    {
        return ::izenelib::util::Singleton<JobScheduler>::get();
    }

    void addTask(task_type task);

private:
    void runAsynchronousTasks_();

private:
    ::izenelib::util::concurrent_queue<task_type> asynchronousTasks_;

    boost::thread asynchronousWorker_;

};

}

#endif


#ifndef __IMAGE_PROCESS_TASK_H__
#define __IMAGE_PROCESS_TASK_H__
#include "MessageQueue.h"
#include "ScopeLock.h"
#include "Request.h"
#include <vector>
#include <sys/types.h>
#include <pthread.h>
#include <util/singleton.h>
#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>

#define DEFAULT_THREAD_COUNT    1
class ImageProcessTask : public boost::noncopyable
{
public:
    friend class izenelib::util::Singleton<ImageProcessTask>;
public:
    static ImageProcessTask* instance()
    {
        return izenelib::util::Singleton<ImageProcessTask>::get();
    }
    bool init(const std::string& imgfiledir, ssize_t threadCount=1);
    bool start();
    bool stop();
    bool put(MSG msg);
    size_t getQueueSize(){
        return msg_queue_.getSize();
    }
    ~ImageProcessTask();

private:
    ImageProcessTask(){}
    static void *process(void*);
    static void *compute(void *arg);
    int doJobs(const MSG& msg);
    int doCompute(const std::string& img_file);
private:
    MessageQueue<MSG> msg_queue_;
    MessageQueue<std::string> compute_queue_;
    bool _stop;
    std::vector<pthread_t> io_threadIds_;
    std::vector<pthread_t> compute_threadIds_;
    std::string  img_file_dir_;
    int   cur_writter_;
    bool  need_backup_;
    boost::mutex  backup_db_lock_;

};

#endif

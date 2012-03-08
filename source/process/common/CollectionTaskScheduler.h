/**
 * @file CollectionTaskScheduler.h
 * @author Zhongxia Li
 * @date Mar 6, 2012
 * @brief 
 */
#ifndef SF1R_PROCESS_COMMON_COLLECTION_TASK_SCHEDULER_H_
#define SF1R_PROCESS_COMMON_COLLECTION_TASK_SCHEDULER_H_

#include <vector>

#include <util/singleton.h>
#include <util/scheduler.h>

namespace sf1r
{

class CollectionTaskType;
class CollectionHandler;

class CollectionTaskScheduler
{
public:
    CollectionTaskScheduler();

    ~CollectionTaskScheduler();

    static CollectionTaskScheduler* get()
    {
        return izenelib::util::Singleton<CollectionTaskScheduler>::get();
    }

    bool schedule(const CollectionHandler* collectionHandler);

private:
    void cronTask_();

    void onTimer_();

private:
    typedef std::vector<boost::shared_ptr<CollectionTaskType> > TaskListType;
    TaskListType taskList_;
};

}

#endif /* SF1R_PROCESS_COMMON_COLLECTION_TASK_SCHEDULER_H_ */

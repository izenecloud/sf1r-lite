#ifndef SF1R_PROCESS_COMMON_COLLECTION_TASK_H_
#define SF1R_PROCESS_COMMON_COLLECTION_TASK_H_

#include <iostream>
#include <string>

#include <util/cronexpression.h>

#include <common/JobScheduler.h>

namespace sf1r
{

class CollectionTaskType
{
public:
    CollectionTaskType(const std::string& collectionName)
        : collectionName_(collectionName)
        , isCronTask_(false)
        , isRunning_(false)
    {}

    virtual ~CollectionTaskType() {}

    void setIsCronTask(bool isCronTask)
    {
        isCronTask_ = isCronTask;
    }

    bool isCronTask()
    {
        return isCronTask_;
    }

    bool setCronExpression(const std::string& expr)
    {
        cronExpressionStr_ = expr;
        return cronExpression_.setExpression(expr);
    }

    const std::string& getCronExpressionStr()
    {
        return cronExpressionStr_;
    }

    const izenelib::util::CronExpression& getCronExpression()
    {
        return cronExpression_;
    }

public:
    virtual void startTask() = 0;

protected:
    std::string collectionName_;

    bool isCronTask_;
    std::string cronExpressionStr_;
    izenelib::util::CronExpression cronExpression_;

    bool isRunning_;

    JobScheduler asyncJodScheduler_;
};

class DocumentManager;
class RebuildTask : public CollectionTaskType
{
public:
    RebuildTask(const std::string& collectionName)
        : CollectionTaskType(collectionName)
    {
        rebuildCollectionName_ = collectionName + "-rebuild";
    }

    virtual void startTask();

private:
    void doTask();

private:
    std::string rebuildCollectionName_;
};

}

#endif /* SF1R_PROCESS_COMMON_COLLECTION_TASK_H_ */

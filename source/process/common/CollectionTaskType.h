#ifndef SF1R_PROCESS_COMMON_COLLECTION_TASK_TYPE_H_
#define SF1R_PROCESS_COMMON_COLLECTION_TASK_TYPE_H_

#include <iostream>
#include <string>

#include <util/cronexpression.h>
#include <common/Utilities.h>

namespace sf1r
{

class CollectionTaskType
{
public:
    CollectionTaskType(const std::string& collectionName)
        : collectionName_(collectionName)
        , isCronTask_(false)
        , isRunning_(false)
        , lastStart_(0)
        , checkInterval_(7*1000)
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

    /**
     * @brief The minimum interval for next check once task started
     * @param checkInterval in seconds
     */
    void setCheckInterval(unsigned int checkInterval)
    {
        checkInterval_ = checkInterval;
    }

    unsigned int getCheckInterval()
    {
        return checkInterval_;
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

    bool checkStart()
    {
        if (isRunning_)
        {
            return false;
        }

        time_t now = Utilities::createTimeStamp();
        time_t interval = now - lastStart_;

        if (lastStart_ > 0 && interval < checkInterval_)
        {
            return false;
        }

        lastStart_ = now;
        return true;
    }

    virtual void doTask() = 0;

protected:
    std::string collectionName_;

    bool isCronTask_;
    std::string cronExpressionStr_;
    izenelib::util::CronExpression cronExpression_;

    bool isRunning_;
    time_t lastStart_;
    time_t checkInterval_;
};

class RebuildTask : public CollectionTaskType
{
public:
    RebuildTask(const std::string& collectionName)
        : CollectionTaskType(collectionName)
    {}

    virtual void doTask();
};

}

#endif /* SF1R_PROCESS_COMMON_COLLECTION_TASK_TYPE_H_ */

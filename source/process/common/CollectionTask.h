#ifndef SF1R_PROCESS_COMMON_COLLECTION_TASK_H_
#define SF1R_PROCESS_COMMON_COLLECTION_TASK_H_

#include <iostream>
#include <string>

#include <util/cronexpression.h>
#include <common/JobScheduler.h>

namespace sf1r
{

class CollectionTask
{
public:
    CollectionTask(const std::string& collectionName)
        : collectionName_(collectionName)
        , isCronTask_(false)
    {}

    virtual ~CollectionTask() {}

    void setIsCronTask(bool isCronTask)
    {
        isCronTask_ = isCronTask;
    }

    virtual std::string getTaskName() const = 0;

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

    void cronTask(int calltype);

public:
    virtual void doTask() = 0;

protected:
    std::string collectionName_;

    bool isCronTask_;
    std::string cronExpressionStr_;
    izenelib::util::CronExpression cronExpression_;
};

class DocumentManager;
class RebuildTask : public CollectionTask
{
public:
    RebuildTask(const std::string& collectionName)
        : CollectionTask(collectionName)
    {
        rebuildCollectionName_ = collectionName + "-rebuild";
    }

    virtual std::string getTaskName() const
    {
        return rebuildCollectionName_;
    }
    virtual void doTask();

private:
    std::string rebuildCollectionName_;
};

class ExpirationCheckTask : public CollectionTask
{
public:
	ExpirationCheckTask(const std::string collectionName,
						std::pair<uint32_t, uint32_t> licenseDate)
		: CollectionTask(collectionName)
	{
        taskName_ = collectionName + "-expiration";
		startDate_ = licenseDate.first;
		endDate_ = licenseDate.second;
	}

    virtual std::string getTaskName() const
    {
        return taskName_;
    }
    virtual void doTask();

private:
	bool checkCollectionHandler(const std::string& collectionName) const;

private:
    std::string taskName_;
	uint32_t startDate_;
	uint32_t endDate_;
};

}

#endif /* SF1R_PROCESS_COMMON_COLLECTION_TASK_H_ */

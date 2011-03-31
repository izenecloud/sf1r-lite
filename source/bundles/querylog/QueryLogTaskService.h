#ifndef QUERYLOG_BUNDLE_TASK_SERVICE_H
#define QUERYLOG_BUNDLE_TASK_SERVICE_H

#include <util/osgi/IService.h>

#include <mining-manager/MiningManager.h>

namespace sf1r
{
class QueryLogTaskService : public ::izenelib::osgi::IService
{
public:
    QueryLogTaskService();

    ~QueryLogTaskService();

    void DoMiningCollection();

private:
    boost::shared_ptr<MiningManager> miningManager_;

    friend class QueryLogBundleActivator;
};

}

#endif


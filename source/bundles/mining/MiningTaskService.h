#ifndef MINING_BUNDLE_TASK_SERVICE_H
#define MINING_BUNDLE_TASK_SERVICE_H

#include <util/osgi/IService.h>

#include <mining-manager/MiningManager.h>

namespace sf1r
{
class MiningTaskService : public ::izenelib::osgi::IService
{
public:
    MiningTaskService();

    ~MiningTaskService();

    void buildCollection(unsigned int numdoc);

    void optimizeIndex();

private:
    boost::shared_ptr<MiningManager> miningManager_;

    friend class MiningBundleActivator;
};

}

#endif


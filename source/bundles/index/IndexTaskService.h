#ifndef INDEX_BUNDLE_TASK_SERVICE_H
#define INDEX_BUNDLE_TASK_SERVICE_H

#include "IndexBundleConfiguration.h"

#include <directory-manager/DirectoryRotator.h>
#include <common/Status.h>
#include <common/ScdParser.h>

#include <util/driver/Value.h>
#include <util/osgi/IService.h>
#include <util/cronexpression.h>

#include <boost/shared_ptr.hpp>

namespace sf1r
{
using izenelib::ir::idmanager::IDManager;

class IndexAggregator;
class IndexWorker;

class IndexTaskService : public ::izenelib::osgi::IService
{
public:
    IndexTaskService(IndexBundleConfiguration* bundleConfig);

    ~IndexTaskService();

    bool index(unsigned int numdoc);

    bool optimizeIndex();

    bool optimizeIndexIds();

    bool createDocument(const ::izenelib::driver::Value& documentValue);

    bool updateDocument(const ::izenelib::driver::Value& documentValue);

    bool destroyDocument(const ::izenelib::driver::Value& documentValue);

    bool getIndexStatus(Status& status);

    uint32_t getDocNum();
    
    uint32_t getKeyCount(const std::string& property_name);

    std::string getScdDir() const;

private:
    bool indexMaster_(unsigned int numdoc);

    void cronJob_();

private:
    IndexBundleConfiguration* bundleConfig_;

    boost::shared_ptr<IndexAggregator> indexAggregator_;
    boost::shared_ptr<IndexWorker> indexWorker_;

    izenelib::util::CronExpression cronExpression_;
    const std::string cronJobName_;

    friend class WorkerServer;
    friend class IndexBundleActivator;
    friend class ProductBundleActivator;
};

}

#endif

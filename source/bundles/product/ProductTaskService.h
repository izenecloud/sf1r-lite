#ifndef PRODUCT_BUNDLE_TASK_SERVICE_H
#define PRODUCT_BUNDLE_TASK_SERVICE_H

#include "ProductBundleConfiguration.h"

#include <configuration-manager/PropertyConfig.h>
#include <configuration-manager/ConfigurationTool.h>
#include <common/Status.h>

#include <util/osgi/IService.h>
#include <util/driver/Value.h>

#include <boost/shared_ptr.hpp>

namespace sf1r
{

class ProductTaskService : public ::izenelib::osgi::IService
{

public:
    ProductTaskService(
        ProductBundleConfiguration* bundleConfig);

    ~ProductTaskService();

    ProductBundleConfiguration* getbundleConfig()
    {
        return bundleConfig_;
    }


private:
    ProductBundleConfiguration* bundleConfig_;

    friend class ProductBundleActivator;
};

}

#endif

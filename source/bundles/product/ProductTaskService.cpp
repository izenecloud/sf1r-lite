#include "ProductTaskService.h"
#include <log-manager/ProductCount.h>
#include <common/Utilities.h>
#include <license-manager/LicenseManager.h>
#include <util/profiler/ProfilerGroup.h>
#include <la/util/UStringUtil.h>

namespace sf1r
{

ProductTaskService::ProductTaskService(
    ProductBundleConfiguration* bundleConfig
    )
    : bundleConfig_(bundleConfig)

{
}

ProductTaskService::~ProductTaskService()
{
}

}

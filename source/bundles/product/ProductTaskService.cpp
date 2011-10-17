#include "ProductTaskService.h"
#include <bundles/index/IndexTaskService.h>

#include <index-manager/IndexManager.h>
#include <document-manager/DocumentManager.h>

#include <log-manager/ProductInfo.h>

#include <common/SFLogger.h>
#include <common/Utilities.h>
#include <license-manager/LicenseManager.h>
#include <process/common/CollectionMeta.h>
#include <process/common/XmlConfigParser.h>

#include <util/profiler/ProfilerGroup.h>

#include <boost/filesystem.hpp>

#include <la/util/UStringUtil.h>

#include <glog/logging.h>

#include <memory> // for auto_ptr
#include <signal.h>
#include <protect/RestrictMacro.h>
namespace bfs = boost::filesystem;


namespace
{
/** the directory for scd file backup */
const char* SCD_BACKUP_DIR = "backup";
}

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


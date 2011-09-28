#include "ProductSearchService.h"

#include <index-manager/IndexManager.h>
#include <document-manager/DocumentManager.h>

#include <common/SFLogger.h>

#include <util/profiler/Profiler.h>
#include <util/profiler/ProfilerGroup.h>
#include <util/singleton.h>
#include <util/get.h>

using namespace izenelib::util;


namespace sf1r
{

ProductSearchService::ProductSearchService(ProductBundleConfiguration* config)
{
    bundleConfig_ = config;
}

ProductSearchService::~ProductSearchService()
{
}

bool ProductSearchService::getDocumentsByIds(
    const GetDocumentsByIdsActionItem& actionItem,
    RawTextResultFromSIA& resultItem
)
{
    return true;
}

}


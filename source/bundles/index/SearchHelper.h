#ifndef BUNDLE_INDEX_SEARCH_HELPER_H
#define BUNDLE_INDEX_SEARCH_HELPER_H

#include <query-manager/SearchKeywordOperation.h>
#include "IndexBundleConfiguration.h"

namespace sf1r
{
bool buildQueryTree(SearchKeywordOperation&action, IndexBundleConfiguration& bundleConfig, std::string& btqError);

}

#endif

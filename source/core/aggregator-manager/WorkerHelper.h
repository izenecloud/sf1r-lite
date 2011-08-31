#ifndef WORKER_HELPER_H
#define WORKER_HELPER_H

#include <bundles/index/IndexBundleConfiguration.h>
#include <query-manager/SearchKeywordOperation.h>
#include <common/ResultType.h>
#include <search-manager/PersonalizedSearchInfo.h>

#include <ir/index_manager/index/IndexerDocument.h>

namespace sf1r
{
void assembleConjunction(std::vector<izenelib::util::UString> keywords, std::string& result);
void assembleDisjunction(std::vector<izenelib::util::UString> keywords, std::string& result);

bool buildQueryTree(
        SearchKeywordOperation&action,
        IndexBundleConfiguration& bundleConfig,
        std::string& btqError,
        PersonalSearchInfo& personalSearchInfo);
}

#endif

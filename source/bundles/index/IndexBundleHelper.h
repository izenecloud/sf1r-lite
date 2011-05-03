#ifndef BUNDLE_INDEX_BUNDLE_HELPER_H
#define BUNDLE_INDEX_BUNDLE_HELPER_H

#include "IndexBundleConfiguration.h"
#include <query-manager/SearchKeywordOperation.h>
#include <search-manager/PersonalizedSearchInfo.h>

#include <ir/index_manager/index/IndexerDocument.h>

namespace sf1r
{
bool buildQueryTree(SearchKeywordOperation&action, IndexBundleConfiguration& bundleConfig, std::string& btqError, PersonalSearchInfo& personalSearchInfo);
void assembleConjunction(std::vector<izenelib::util::UString> keywords, std::string& result);
void assembleDisjunction(std::vector<izenelib::util::UString> keywords, std::string& result);
void split_string(const izenelib::util::UString& szText, std::list<PropertyType>& out, izenelib::util::UString::EncodingType encoding, char Separator = ' ');
void split_int(const izenelib::util::UString& szText, std::list<PropertyType>& out, izenelib::util::UString::EncodingType encoding, char Separator = ' ');
void split_float(const izenelib::util::UString& szText, std::list<PropertyType>& out, izenelib::util::UString::EncodingType encoding, char Separator = ' ');

}

#endif

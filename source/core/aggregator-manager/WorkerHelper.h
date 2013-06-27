#ifndef WORKER_HELPER_H
#define WORKER_HELPER_H

#include <bundles/index/IndexBundleConfiguration.h>
#include <query-manager/SearchKeywordOperation.h>
#include <common/ResultType.h>
#include <search-manager/PersonalizedSearchInfo.h>

#include <ir/index_manager/index/IndexerDocument.h>

namespace sf1r
{
bool buildQueryTree(
        SearchKeywordOperation&action,
        IndexBundleConfiguration& bundleConfig,
        std::string& btqError,
        PersonalSearchInfo& personalSearchInfo);

//void split_string(const izenelib::util::UString& szText, std::list<PropertyType>& out, izenelib::util::UString::EncodingType encoding, char Separator = ' ');
//void split_int32(const izenelib::util::UString& szText, std::list<PropertyType>& out, izenelib::util::UString::EncodingType encoding, const char* sep = " ");
//void split_int64(const izenelib::util::UString& szText, std::list<PropertyType>& out, izenelib::util::UString::EncodingType encoding, const char* sep = " ");
//void split_float(const izenelib::util::UString& szText, std::list<PropertyType>& out, izenelib::util::UString::EncodingType encoding, const char* sep = " ");
//void split_datetime(const izenelib::util::UString& szText, std::list<PropertyType>& out, izenelib::util::UString::EncodingType encoding, const char* sep = " ");

} //namespace

#endif

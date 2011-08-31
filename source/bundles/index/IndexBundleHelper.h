#ifndef BUNDLE_INDEX_BUNDLE_HELPER_H
#define BUNDLE_INDEX_BUNDLE_HELPER_H

#include "IndexBundleConfiguration.h"

#include <ir/index_manager/index/IndexerDocument.h>

namespace sf1r
{
void split_string(const izenelib::util::UString& szText, std::list<PropertyType>& out, izenelib::util::UString::EncodingType encoding, char Separator = ' ');
void split_int(const izenelib::util::UString& szText, std::list<PropertyType>& out, izenelib::util::UString::EncodingType encoding, char Separator = ' ');
void split_float(const izenelib::util::UString& szText, std::list<PropertyType>& out, izenelib::util::UString::EncodingType encoding, char Separator = ' ');

}

#endif

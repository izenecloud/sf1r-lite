#ifndef INDEX_BUNDLE_SEARCH_SERVICE_H
#define INDEX_BUNDLE_SEARCH_SERVICE_H

#include <util/osgi/IService.h>

#include <common/sf1_serialization_types.h>
#include <common/type_defs.h>

#include <configuration-manager/SiaConfig.h>

#include <search-manager/SearchManager.h>
#include <ranking-manager/RankingManager.h>
#include <query-manager/ActionItem.h>
#include <question-answering/QuestionAnalysis.h>

namespace sf1r
{
class IndexSearchService : public ::izenelib::osgi::IService
{
public:
    IndexSearchService();

    ~IndexSearchService();

public:
    bool getSearchResult(KeywordSearchActionItem& actionItem, KeywordSearchResult& resultItem);

    bool getDocumentsByIds(const GetDocumentsByIdsActionItem& actionItem, RawTextResultFromSIA& resultItem);

    bool getInternalDocumentId(const izenelib::util::UString& scdDocumentId, uint32_t& internalId);

};

}

#endif


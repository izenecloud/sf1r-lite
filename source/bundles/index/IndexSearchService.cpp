#include "IndexSearchService.h"

namespace sf1r
{
int TOP_K_NUM = 1000;

IndexSearchService::IndexSearchService()
{
}

IndexSearchService::~IndexSearchService()
{
}


bool IndexSearchService::getSearchResult(
    KeywordSearchActionItem& actionItem, 
    KeywordSearchResult& resultItem
)
{
    return true;
}

bool IndexSearchService::getDocumentsByIds(
    const GetDocumentsByIdsActionItem& actionItem, 
    RawTextResultFromSIA& resultItem
)
{
    return true;
}

bool IndexSearchService::getInternalDocumentId(
    const izenelib::util::UString& scdDocumentId, 
    uint32_t& internalId
)
{
    //izenelib::util::UString scdDocumentId;

    //std::string str;
    //scdDocumentId.convertString(str, izenelib::util::UString::UTF_8);
    //idManager_->getDocIdByDocName(scdDocumentId, internalId);
    return true;
}

bool IndexSearchService::getIndexStatus(Status& status)
{
    return true;
}

}


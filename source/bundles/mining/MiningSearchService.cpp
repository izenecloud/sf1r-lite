#include "MiningSearchService.h"

namespace sf1r
{

MiningSearchService::MiningSearchService()
{
}

MiningSearchService::~MiningSearchService()
{
}


bool MiningSearchService::getSearchResult(
    KeywordSearchResult& resultItem
)
{
    return true;
}

bool MiningSearchService::getSimilarDocIdList(
    uint32_t documentId, 
    uint32_t maxNum, 
    std::vector<std::pair<uint32_t, float> >& result
)
{
    return true;
}

bool MiningSearchService::getDuplicateDocIdList(
    uint32_t docId, 
    std::vector<uint32_t>& docIdList
)
{
    return true;
}

bool MiningSearchService::getSimilarImageDocIdList(
    const std::string& targetImageURI,
    SimilarImageDocIdList& imageDocIdList
)
{
    return true;
}

}

